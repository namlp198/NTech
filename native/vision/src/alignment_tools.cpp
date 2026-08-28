#include "alignment_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/feature.hpp"
#include "nvision/core/measurement.hpp"

#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::CoordinateSpaceId;
using core::Feature;
using core::FeatureMatch;
using core::FeatureSet;
using core::Line2D;
using core::MatchSet;
using core::Measurement;
using core::MeasurementKey;
using core::MeasurementList;
using core::MeasurementUnit;
using core::ParameterKey;
using core::Point2D;
using core::PortKey;
using core::Transform2D;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

std::array<double, 9> as_array(const cv::Matx33d& matrix) {
    return std::array<double, 9>{
        matrix(0, 0), matrix(0, 1), matrix(0, 2),
        matrix(1, 0), matrix(1, 1), matrix(1, 2),
        matrix(2, 0), matrix(2, 1), matrix(2, 2)};
}

cv::Matx33d as_matrix(const Transform2D& transform) {
    const std::array<double, 9>& values = transform.matrix();
    return cv::Matx33d(
        values[0], values[1], values[2],
        values[3], values[4], values[5],
        values[6], values[7], values[8]);
}

class LineFitTool final : public VisionTool {
public:
    LineFitTool() : VisionTool("nvision.tool.geometry.line-fit", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        if (points.size() < 2U) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS", "fitting a line needs at least two points"));
        }

        const std::string method =
            context.parameter(ParameterKey("method")).enumeration_value().value();
        const int distance_type = method == "huber" ? cv::DIST_HUBER : cv::DIST_L2;

        cv::Vec4f fitted;
        cv::fitLine(as_opencv_points(points), fitted, distance_type, 0.0, 0.01, 0.01);
        const cv::Point2d direction(fitted[0], fitted[1]);
        const cv::Point2d anchor(fitted[2], fitted[3]);

        // The reported segment spans the points that were fitted, which is more useful
        // than an arbitrary unit segment and guarantees two distinct endpoints.
        double lowest = std::numeric_limits<double>::max();
        double highest = std::numeric_limits<double>::lowest();
        double residual = 0.0;
        for (const Point2D& point : points) {
            const double along =
                (point.x() - anchor.x) * direction.x + (point.y() - anchor.y) * direction.y;
            lowest = std::min(lowest, along);
            highest = std::max(highest, along);
            residual += std::abs(
                (point.x() - anchor.x) * direction.y - (point.y() - anchor.y) * direction.x);
        }
        if (highest - lowest <= 0.0) {
            return ToolExecutionOutcome::failed(configuration_error(
                "DEGENERATE_POINTS", "every supplied point is at the same position"));
        }

        const CoordinateSpaceId& space = points.front().coordinate_space();
        const Line2D line(
            Point2D(space, anchor.x + direction.x * lowest, anchor.y + direction.y * lowest),
            Point2D(space, anchor.x + direction.x * highest, anchor.y + direction.y * highest));

        PortValueMap outputs;
        outputs.set(PortKey("line"), PortValue::line(line));
        outputs.set(
            PortKey("residual"),
            PortValue::floating_point(residual / static_cast<double>(points.size())));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class FeatureDetectAndMatchTool final : public VisionTool {
public:
    FeatureDetectAndMatchTool()
        : VisionTool("nvision.tool.geometry.feature-detect-and-match", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const ImageView& reference = context.input(PortKey("reference")).image_value();
        if (image.pixel_format() != PixelFormat::mono8 ||
            reference.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "feature matching requires 8-bit single-channel images"));
        }
        if (image.coordinate_space() == reference.coordinate_space()) {
            // The result would be an identity that looks like a successful alignment.
            return ToolExecutionOutcome::failed(configuration_error(
                "SAME_COORDINATE_SPACE",
                "the image and the reference are the same raster, so matching them is empty"));
        }

        const std::string backend =
            context.parameter(ParameterKey("backend")).enumeration_value().value();
        const auto maximum_features = static_cast<int>(
            context.parameter(ParameterKey("maximum-features")).integer_value());
        const double ratio =
            context.parameter(ParameterKey("ratio-threshold")).floating_point_value();

        cv::Ptr<cv::Feature2D> detector;
        int norm = cv::NORM_HAMMING;
        if (backend == "orb") {
            detector = cv::ORB::create(maximum_features);
        } else if (backend == "sift") {
            detector = cv::SIFT::create(maximum_features);
            // SIFT descriptors are floating point, so Hamming distance is meaningless.
            norm = cv::NORM_L2;
        } else {
            detector = cv::AKAZE::create();
        }

        std::vector<cv::KeyPoint> image_points;
        std::vector<cv::KeyPoint> reference_points;
        cv::Mat image_descriptors;
        cv::Mat reference_descriptors;
        detector->detectAndCompute(
            interop::borrow(image), cv::noArray(), image_points, image_descriptors);
        detector->detectAndCompute(
            interop::borrow(reference), cv::noArray(), reference_points, reference_descriptors);

        FeatureSet query = features_of(image.coordinate_space(), image_points);
        FeatureSet target = features_of(reference.coordinate_space(), reference_points);

        std::vector<FeatureMatch> accepted;
        if (!image_descriptors.empty() && !reference_descriptors.empty()) {
            std::vector<std::vector<cv::DMatch>> candidates;
            cv::BFMatcher(norm).knnMatch(image_descriptors, reference_descriptors, candidates, 2);
            for (const std::vector<cv::DMatch>& pair : candidates) {
                // Lowe's ratio test: a match is only trusted when it is clearly closer
                // than the next best candidate for the same feature.
                if (pair.size() < 2U || pair[0].distance >= ratio * pair[1].distance) {
                    continue;
                }
                accepted.emplace_back(
                    static_cast<std::size_t>(pair[0].queryIdx),
                    static_cast<std::size_t>(pair[0].trainIdx),
                    static_cast<double>(pair[0].distance));
            }
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("matches"),
            PortValue::match_set(MatchSet(query, std::move(target), std::move(accepted))));
        outputs.set(PortKey("features"), PortValue::feature_set(std::move(query)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }

private:
    static FeatureSet features_of(
        const CoordinateSpaceId& space,
        const std::vector<cv::KeyPoint>& key_points) {
        std::vector<Feature> features;
        features.reserve(key_points.size());
        for (const cv::KeyPoint& key_point : key_points) {
            features.emplace_back(
                point_in(space, key_point.pt),
                // A detector may report a zero size for a degenerate keypoint, and a
                // feature contract requires a positive one.
                key_point.size > 0.0F ? static_cast<double>(key_point.size) : 1.0,
                static_cast<double>(key_point.angle),
                static_cast<double>(key_point.response));
        }
        return FeatureSet(space, std::move(features));
    }
};

class HomographyEstimateTool final : public VisionTool {
public:
    HomographyEstimateTool()
        : VisionTool("nvision.tool.geometry.homography-estimate", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const MatchSet& matches = context.input(PortKey("matches")).match_set_value();
        // Four correspondences are the minimum a projective mapping is determined by.
        if (matches.size() < 4U) {
            return ToolExecutionOutcome::failed(vision_error(
                "NOT_ENOUGH_MATCHES",
                "estimating a homography needs at least four correspondences, and " +
                    std::to_string(matches.size()) + " were accepted"));
        }

        std::vector<cv::Point2f> from;
        std::vector<cv::Point2f> to;
        from.reserve(matches.size());
        to.reserve(matches.size());
        for (std::size_t index = 0U; index < matches.size(); ++index) {
            const Point2D& query = matches.query_point(index);
            const Point2D& reference = matches.reference_point(index);
            from.emplace_back(static_cast<float>(query.x()), static_cast<float>(query.y()));
            to.emplace_back(static_cast<float>(reference.x()), static_cast<float>(reference.y()));
        }

        const std::string method =
            context.parameter(ParameterKey("method")).enumeration_value().value();
        const double threshold =
            context.parameter(ParameterKey("reprojection-threshold")).floating_point_value();

        cv::Mat inliers;
        const cv::Mat estimated = cv::findHomography(
            from, to, method == "lmeds" ? cv::LMEDS : cv::RANSAC, threshold, inliers);
        if (estimated.empty()) {
            return ToolExecutionOutcome::failed(vision_error(
                "ESTIMATION_FAILED",
                "no homography agreed with enough of the supplied correspondences"));
        }

        const double inlier_ratio =
            inliers.empty()
                ? 0.0
                : static_cast<double>(cv::countNonZero(inliers)) /
                      static_cast<double>(matches.size());

        PortValueMap outputs;
        outputs.set(
            PortKey("homography"),
            PortValue::homography(Transform2D(
                matches.query().coordinate_space(),
                matches.reference().coordinate_space(),
                as_array(cv::Matx33d(estimated)))));
        outputs.set(PortKey("inlier-ratio"), PortValue::floating_point(inlier_ratio));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class HomographyValidateTool final : public VisionTool {
public:
    HomographyValidateTool()
        : VisionTool("nvision.tool.geometry.homography-validate", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const Transform2D& homography = context.input(PortKey("homography")).transform_value();
        const std::array<double, 9>& values = homography.matrix();

        // A plausible alignment neither collapses the plane nor stretches one axis far
        // more than the other, so both are measured and reported alongside the verdict.
        const double determinant = values[0] * values[4] - values[1] * values[3];
        const double x_scale = std::hypot(values[0], values[3]);
        const double y_scale = std::hypot(values[1], values[4]);
        const double larger = std::max(x_scale, y_scale);
        const double deviation = larger > 0.0 ? std::abs(x_scale - y_scale) / larger : 1.0;

        const double minimum_determinant =
            context.parameter(ParameterKey("minimum-determinant")).floating_point_value();
        const double maximum_deviation =
            context.parameter(ParameterKey("maximum-scale-deviation")).floating_point_value();
        const bool valid =
            std::abs(determinant) >= minimum_determinant && deviation <= maximum_deviation;

        MeasurementList measurements;
        measurements.emplace_back(
            MeasurementKey("homography.determinant"), determinant, MeasurementUnit::none);
        measurements.emplace_back(
            MeasurementKey("homography.scale-deviation"), deviation, MeasurementUnit::ratio);

        PortValueMap outputs;
        outputs.set(PortKey("valid"), PortValue::boolean(valid));
        outputs.set(
            PortKey("measurements"), PortValue::measurement_list(std::move(measurements)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class EccRefineTool final : public VisionTool {
public:
    EccRefineTool() : VisionTool("nvision.tool.geometry.ecc-refine", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const ImageView& reference = context.input(PortKey("reference")).image_value();
        const Transform2D& initial = context.input(PortKey("transform")).transform_value();

        if (image.pixel_format() != PixelFormat::mono8 ||
            reference.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "ECC refinement requires 8-bit single-channel images"));
        }
        if (initial.source_space() != image.coordinate_space() ||
            initial.target_space() != reference.coordinate_space()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the starting transform must map the image space to the reference space"));
        }

        const auto iterations = static_cast<int>(
            context.parameter(ParameterKey("iterations")).integer_value());
        const double epsilon = context.parameter(ParameterKey("epsilon")).floating_point_value();

        cv::Mat warp;
        cv::Mat(as_matrix(initial)).convertTo(warp, CV_32F);

        double correlation = 0.0;
        try {
            correlation = cv::findTransformECC(
                interop::borrow(reference),
                interop::borrow(image),
                warp,
                cv::MOTION_HOMOGRAPHY,
                cv::TermCriteria(
                    cv::TermCriteria::COUNT + cv::TermCriteria::EPS, iterations, epsilon),
                cv::noArray());
        } catch (const cv::Exception&) {
            // Failing to converge is an outcome of this frame, not a defect in the graph,
            // so it is reported as a recoverable vision error rather than a fault.
            return ToolExecutionOutcome::failed(vision_error(
                "REFINEMENT_DID_NOT_CONVERGE",
                "ECC refinement did not converge from the supplied starting transform"));
        }

        cv::Mat refined;
        warp.convertTo(refined, CV_64F);

        PortValueMap outputs;
        outputs.set(
            PortKey("transform"),
            PortValue::transform(Transform2D(
                initial.source_space(), initial.target_space(), as_array(cv::Matx33d(refined)))));
        outputs.set(PortKey("correlation"), PortValue::floating_point(correlation));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class PointTransformTool final : public VisionTool {
public:
    PointTransformTool()
        : VisionTool("nvision.tool.geometry.point-transform", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        const Transform2D& transform = context.input(PortKey("transform")).transform_value();

        if (!points.empty() && points.front().coordinate_space() != transform.source_space()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the transform maps from " + transform.source_space().value() +
                    " but the points are in " + points.front().coordinate_space().value()));
        }

        std::vector<Point2D> transformed;
        transformed.reserve(points.size());
        for (const Point2D& point : points) {
            transformed.push_back(transform.apply(point));
        }

        PortValueMap outputs;
        outputs.set(PortKey("points"), PortValue::point_list(std::move(transformed)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class PointAssignmentTool final : public VisionTool {
public:
    PointAssignmentTool()
        : VisionTool("nvision.tool.geometry.point-assignment", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        const auto& reference = context.input(PortKey("reference")).point_list_value();

        if (!points.empty() && !reference.empty() &&
            points.front().coordinate_space() != reference.front().coordinate_space()) {
            // Comparing distances across spaces would produce numbers with no meaning.
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the points and the reference are in different coordinate spaces"));
        }

        const double maximum_distance =
            context.parameter(ParameterKey("maximum-distance")).floating_point_value();

        // Reference points are matched in order and a candidate is consumed once, so the
        // same detected point cannot satisfy two expected positions and a repeated run
        // produces an identical assignment.
        std::vector<bool> taken(points.size(), false);
        MeasurementList assignments;
        for (std::size_t expected = 0U; expected < reference.size(); ++expected) {
            std::size_t nearest = points.size();
            double nearest_distance = maximum_distance;
            for (std::size_t candidate = 0U; candidate < points.size(); ++candidate) {
                if (taken[candidate]) {
                    continue;
                }
                const double distance = std::hypot(
                    points[candidate].x() - reference[expected].x(),
                    points[candidate].y() - reference[expected].y());
                if (distance <= nearest_distance) {
                    nearest = candidate;
                    nearest_distance = distance;
                }
            }
            if (nearest == points.size()) {
                continue;
            }
            taken[nearest] = true;
            assignments.emplace_back(
                MeasurementKey("assignment." + std::to_string(expected) + ".distance"),
                nearest_distance,
                MeasurementUnit::pixel,
                reference[expected].coordinate_space());
        }

        std::vector<Point2D> unmatched;
        for (std::size_t candidate = 0U; candidate < points.size(); ++candidate) {
            if (!taken[candidate]) {
                unmatched.push_back(points[candidate]);
            }
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("assignments"), PortValue::measurement_list(std::move(assignments)));
        outputs.set(PortKey("unmatched"), PortValue::point_list(std::move(unmatched)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_line_fit_tool() {
    return std::make_shared<const LineFitTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_feature_detect_and_match_tool() {
    return std::make_shared<const FeatureDetectAndMatchTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_homography_estimate_tool() {
    return std::make_shared<const HomographyEstimateTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_homography_validate_tool() {
    return std::make_shared<const HomographyValidateTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_ecc_refine_tool() {
    return std::make_shared<const EccRefineTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_point_transform_tool() {
    return std::make_shared<const PointTransformTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_point_assignment_tool() {
    return std::make_shared<const PointAssignmentTool>();
}

}  // namespace nvision::vision
