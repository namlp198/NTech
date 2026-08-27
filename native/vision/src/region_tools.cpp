#include "region_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/detection.hpp"
#include "nvision/core/measurement.hpp"

#include <opencv2/imgproc.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::Contour;
using core::CoordinateSpaceId;
using core::Detection;
using core::DetectionList;
using core::Measurement;
using core::MeasurementKey;
using core::MeasurementList;
using core::MeasurementUnit;
using core::ParameterKey;
using core::PortKey;
using core::Point2D;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

bool is_mask(const ImageView& image) noexcept {
    return image.pixel_format() == PixelFormat::mono8;
}

core::ExecutionError not_a_mask(const std::string& tool) {
    return configuration_error(
        "UNSUPPORTED_PIXEL_FORMAT", tool + " requires an 8-bit mask");
}

// Every region a tool reports lives in the coordinate space of the mask it was found in,
// never in the space of some earlier raster the mask was derived from.
Detection detection_of(
    const CoordinateSpaceId& space,
    const cv::Rect& bounds,
    const cv::Point2d& centroid,
    const double area) {
    return Detection(
        core::AxisAlignedRectangle(
            space,
            static_cast<double>(bounds.x),
            static_cast<double>(bounds.y),
            static_cast<double>(bounds.width),
            static_cast<double>(bounds.height)),
        Point2D(space, centroid.x, centroid.y),
        area);
}

std::vector<std::vector<cv::Point>> find_contours(const ImageView& mask, const int mode) {
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // findContours may modify its input on some builds, so it is given a copy rather than
    // the borrowed view, whose pixels are immutable.
    cv::Mat working = interop::borrow(mask).clone();
    cv::findContours(working, contours, hierarchy, mode, cv::CHAIN_APPROX_SIMPLE);
    return contours;
}

class ConnectedComponentsTool final : public VisionTool {
public:
    ConnectedComponentsTool()
        : VisionTool("nvision.tool.geometry.connected-components", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& mask = context.input(PortKey("mask")).image_value();
        if (!is_mask(mask)) {
            return ToolExecutionOutcome::failed(not_a_mask("connected components"));
        }

        const std::string connectivity =
            context.parameter(ParameterKey("connectivity")).enumeration_value().value();
        const int neighbours = connectivity == "four" ? 4 : 8;
        const double minimum_area =
            context.parameter(ParameterKey("minimum-area")).floating_point_value();

        cv::Mat labels;
        cv::Mat statistics;
        cv::Mat centroids;
        const int total = cv::connectedComponentsWithStats(
            interop::borrow(mask), labels, statistics, centroids, neighbours, CV_32S);

        DetectionList components;
        // Label 0 is the background, which is not a component the graph asked for.
        for (int label = 1; label < total; ++label) {
            const auto area = static_cast<double>(statistics.at<int>(label, cv::CC_STAT_AREA));
            if (area < minimum_area) {
                continue;
            }
            const cv::Rect bounds(
                statistics.at<int>(label, cv::CC_STAT_LEFT),
                statistics.at<int>(label, cv::CC_STAT_TOP),
                statistics.at<int>(label, cv::CC_STAT_WIDTH),
                statistics.at<int>(label, cv::CC_STAT_HEIGHT));
            components.push_back(detection_of(
                mask.coordinate_space(),
                bounds,
                cv::Point2d(centroids.at<double>(label, 0), centroids.at<double>(label, 1)),
                area));
        }

        const auto count = static_cast<std::int64_t>(components.size());
        PortValueMap outputs;
        outputs.set(PortKey("components"), PortValue::detection_list(std::move(components)));
        outputs.set(PortKey("count"), PortValue::integer(count));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class ContourExtractionTool final : public VisionTool {
public:
    ContourExtractionTool()
        : VisionTool("nvision.tool.geometry.contour-extraction", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& mask = context.input(PortKey("mask")).image_value();
        if (!is_mask(mask)) {
            return ToolExecutionOutcome::failed(not_a_mask("contour extraction"));
        }

        const std::string mode =
            context.parameter(ParameterKey("mode")).enumeration_value().value();
        int retrieval = cv::RETR_EXTERNAL;
        if (mode == "list") {
            retrieval = cv::RETR_LIST;
        } else if (mode == "tree") {
            retrieval = cv::RETR_TREE;
        }
        const double minimum_area =
            context.parameter(ParameterKey("minimum-area")).floating_point_value();

        std::vector<std::vector<cv::Point>> found;
        std::vector<cv::Vec4i> hierarchy;
        cv::Mat working = interop::borrow(mask).clone();
        cv::findContours(working, found, hierarchy, retrieval, cv::CHAIN_APPROX_SIMPLE);

        // The area filter renumbers the list, so a parent index taken from OpenCV would
        // point at the wrong contour. Deciding what survives first makes the remapping
        // exact, and a contour whose parent was filtered out simply reports no parent.
        std::vector<std::optional<std::size_t>> reported_index(found.size());
        std::size_t surviving = 0U;
        for (std::size_t index = 0U; index < found.size(); ++index) {
            if (cv::contourArea(found[index]) >= minimum_area) {
                reported_index[index] = surviving;
                ++surviving;
            }
        }

        std::vector<Contour> contours;
        contours.reserve(surviving);
        for (std::size_t index = 0U; index < found.size(); ++index) {
            if (!reported_index[index].has_value()) {
                continue;
            }
            std::vector<Point2D> points;
            points.reserve(found[index].size());
            for (const cv::Point& point : found[index]) {
                points.emplace_back(
                    mask.coordinate_space(),
                    static_cast<double>(point.x),
                    static_cast<double>(point.y));
            }
            // Tree mode is the only retrieval that reports enclosure.
            std::optional<std::size_t> parent;
            if (retrieval == cv::RETR_TREE && index < hierarchy.size() &&
                hierarchy[index][3] >= 0) {
                parent = reported_index[static_cast<std::size_t>(hierarchy[index][3])];
            }
            contours.emplace_back(std::move(points), true, parent);
        }

        PortValueMap outputs;
        outputs.set(PortKey("contours"), PortValue::contour_list(std::move(contours)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class BlobAnalysisTool final : public VisionTool {
public:
    BlobAnalysisTool()
        : VisionTool("nvision.tool.geometry.blob-analysis", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& mask = context.input(PortKey("mask")).image_value();
        if (!is_mask(mask)) {
            return ToolExecutionOutcome::failed(not_a_mask("blob analysis"));
        }

        const double minimum_area =
            context.parameter(ParameterKey("minimum-area")).floating_point_value();
        const double maximum_area =
            context.parameter(ParameterKey("maximum-area")).floating_point_value();
        // Zero means no upper bound, which is how the catalog default expresses "any size".
        const bool bounded_above = maximum_area > 0.0;
        if (bounded_above && maximum_area < minimum_area) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_RANGE", "the blob maximum area is below its minimum area"));
        }

        DetectionList blobs;
        MeasurementList measurements;
        std::size_t reported = 0U;
        for (const std::vector<cv::Point>& contour : find_contours(mask, cv::RETR_EXTERNAL)) {
            const double area = cv::contourArea(contour);
            if (area < minimum_area || (bounded_above && area > maximum_area)) {
                continue;
            }
            const cv::Moments moments = cv::moments(contour);
            const cv::Rect bounds = cv::boundingRect(contour);
            // A degenerate contour has no centroid of its own, so the centre of its
            // bounding box stands in rather than a division by zero.
            const cv::Point2d centroid =
                moments.m00 > 0.0
                    ? cv::Point2d(moments.m10 / moments.m00, moments.m01 / moments.m00)
                    : cv::Point2d(
                          bounds.x + bounds.width / 2.0, bounds.y + bounds.height / 2.0);

            blobs.push_back(detection_of(mask.coordinate_space(), bounds, centroid, area));
            measurements.emplace_back(
                MeasurementKey("blob." + std::to_string(reported) + ".area"),
                area,
                MeasurementUnit::square_pixel,
                mask.coordinate_space());
            measurements.emplace_back(
                MeasurementKey("blob." + std::to_string(reported) + ".perimeter"),
                cv::arcLength(contour, true),
                MeasurementUnit::pixel,
                mask.coordinate_space());
            ++reported;
        }

        PortValueMap outputs;
        outputs.set(PortKey("blobs"), PortValue::detection_list(std::move(blobs)));
        outputs.set(
            PortKey("measurements"), PortValue::measurement_list(std::move(measurements)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class MinimumEnclosingCircleTool final : public VisionTool {
public:
    MinimumEnclosingCircleTool()
        : VisionTool(
              "nvision.tool.geometry.minimum-enclosing-circle", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& contours = context.input(PortKey("contours")).contour_list_value();

        std::vector<Point2D> centers;
        MeasurementList radii;
        centers.reserve(contours.size());
        radii.reserve(contours.size());
        for (std::size_t index = 0U; index < contours.size(); ++index) {
            std::vector<cv::Point2f> points = as_opencv_points(contours[index].points());
            cv::Point2f center;
            float radius = 0.0F;
            cv::minEnclosingCircle(points, center, radius);

            centers.push_back(point_in(contours[index].coordinate_space(), center));
            radii.emplace_back(
                MeasurementKey("circle." + std::to_string(index) + ".radius"),
                static_cast<double>(radius),
                MeasurementUnit::pixel,
                contours[index].coordinate_space());
        }

        PortValueMap outputs;
        outputs.set(PortKey("centers"), PortValue::point_list(std::move(centers)));
        outputs.set(PortKey("radii"), PortValue::measurement_list(std::move(radii)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_connected_components_tool() {
    return std::make_shared<const ConnectedComponentsTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_contour_extraction_tool() {
    return std::make_shared<const ContourExtractionTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_blob_analysis_tool() {
    return std::make_shared<const BlobAnalysisTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_minimum_enclosing_circle_tool() {
    return std::make_shared<const MinimumEnclosingCircleTool>();
}

}  // namespace nvision::vision
