#include "morphology_tools.hpp"

#include "tool_support.hpp"

#include <opencv2/imgproc.hpp>

#include <string>
#include <utility>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::ParameterKey;
using core::PortKey;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

class MorphologyTool final : public VisionTool {
public:
    MorphologyTool()
        : VisionTool("nvision.tool.processing.morphology", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& mask = context.input(PortKey("mask")).image_value();
        if (mask.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT", "morphology requires an 8-bit mask"));
        }

        const int kernel = odd_size_parameter(context, ParameterKey("kernel-size"));
        if (kernel == 0) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_KERNEL_SIZE", "a morphology kernel size must be odd"));
        }
        const auto iterations = static_cast<int>(
            context.parameter(ParameterKey("iterations")).integer_value());

        const std::string operation =
            context.parameter(ParameterKey("operation")).enumeration_value().value();
        int morphology_operation = cv::MORPH_OPEN;
        if (operation == "erode") {
            morphology_operation = cv::MORPH_ERODE;
        } else if (operation == "dilate") {
            morphology_operation = cv::MORPH_DILATE;
        } else if (operation == "close") {
            morphology_operation = cv::MORPH_CLOSE;
        }

        const cv::Mat element =
            cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernel, kernel));
        cv::Mat result;
        cv::morphologyEx(
            interop::borrow(mask), result, morphology_operation, element, cv::Point(-1, -1),
            iterations);

        PortValueMap outputs;
        outputs.set(PortKey("mask"), produced_mask(context, PortKey("mask"), result));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class DistanceTransformTool final : public VisionTool {
public:
    DistanceTransformTool()
        : VisionTool("nvision.tool.processing.distance-transform", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& mask = context.input(PortKey("mask")).image_value();
        if (mask.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT", "a distance transform requires an 8-bit mask"));
        }

        const std::string type =
            context.parameter(ParameterKey("distance-type")).enumeration_value().value();
        int distance_type = cv::DIST_L2;
        if (type == "l1") {
            distance_type = cv::DIST_L1;
        } else if (type == "chessboard") {
            distance_type = cv::DIST_C;
        }

        // The result is a distance in pixels, not an intensity, so it stays floating point
        // rather than being rescaled into an 8-bit range that would lose its meaning.
        cv::Mat distances;
        cv::distanceTransform(interop::borrow(mask), distances, distance_type, 3);

        PortValueMap outputs;
        outputs.set(
            PortKey("image"),
            produced_image(context, PortKey("image"), distances, PixelFormat::mono_float32));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_morphology_tool() {
    return std::make_shared<const MorphologyTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_distance_transform_tool() {
    return std::make_shared<const DistanceTransformTool>();
}

}  // namespace nvision::vision
