#include "binarize_tools.hpp"

#include "tool_support.hpp"

#include <opencv2/imgproc.hpp>

#include <string>
#include <utility>
#include <vector>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::ParameterKey;
using core::PortKey;
using workflow::ParameterValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

std::vector<double> bounds_of(const ParameterValue& value) {
    std::vector<double> bounds;
    bounds.reserve(value.array_value().size());
    for (const auto& item : value.array_value()) {
        bounds.push_back(item.floating_point_value());
    }
    return bounds;
}

cv::Scalar as_scalar(const std::vector<double>& bounds) {
    cv::Scalar scalar(0.0, 0.0, 0.0, 0.0);
    for (std::size_t index = 0U; index < bounds.size() && index < 4U; ++index) {
        scalar[static_cast<int>(index)] = bounds[index];
    }
    return scalar;
}

class AdaptiveThresholdTool final : public VisionTool {
public:
    AdaptiveThresholdTool()
        : VisionTool("nvision.tool.processing.adaptive-threshold", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (image.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "adaptive threshold requires an 8-bit single-channel image"));
        }

        const int block = odd_size_parameter(context, ParameterKey("block-size"));
        if (block == 0) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_BLOCK_SIZE", "an adaptive threshold block size must be odd"));
        }

        const std::string method =
            context.parameter(ParameterKey("method")).enumeration_value().value();
        const int adaptive_method = method == "mean" ? cv::ADAPTIVE_THRESH_MEAN_C
                                                     : cv::ADAPTIVE_THRESH_GAUSSIAN_C;
        const double offset = context.parameter(ParameterKey("offset")).floating_point_value();

        cv::Mat mask;
        cv::adaptiveThreshold(
            interop::borrow(image), mask, 255.0, adaptive_method, cv::THRESH_BINARY, block, offset);

        PortValueMap outputs;
        outputs.set(PortKey("mask"), produced_mask(context, PortKey("mask"), mask));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class ColorRangeTool final : public VisionTool {
public:
    ColorRangeTool()
        : VisionTool("nvision.tool.processing.color-range", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const std::vector<double> lower = bounds_of(context.parameter(ParameterKey("lower")));
        const std::vector<double> upper = bounds_of(context.parameter(ParameterKey("upper")));

        const std::size_t channels = is_three_channel(image.pixel_format()) ? 3U : 1U;
        if (lower.size() != channels || upper.size() != channels) {
            // Silently padding or truncating a bound would select a different region than
            // the recipe asked for, which is worse than refusing.
            return ToolExecutionOutcome::failed(configuration_error(
                "BOUND_CHANNEL_MISMATCH",
                "the image has " + std::to_string(channels) +
                    " channels but the bounds do not"));
        }
        for (std::size_t index = 0U; index < channels; ++index) {
            if (lower[index] > upper[index]) {
                return ToolExecutionOutcome::failed(configuration_error(
                    "INVALID_RANGE", "a colour range lower bound exceeds its upper bound"));
            }
        }

        cv::Mat mask;
        cv::inRange(interop::borrow(image), as_scalar(lower), as_scalar(upper), mask);

        PortValueMap outputs;
        outputs.set(PortKey("mask"), produced_mask(context, PortKey("mask"), mask));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_adaptive_threshold_tool() {
    return std::make_shared<const AdaptiveThresholdTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_color_range_tool() {
    return std::make_shared<const ColorRangeTool>();
}

}  // namespace nvision::vision
