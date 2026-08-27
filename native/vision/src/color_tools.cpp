#include "color_tools.hpp"

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

// What one declared conversion expects and produces. Keeping the expectation next to the
// OpenCV code means a mismatched input is reported instead of silently reinterpreted.
struct Conversion final {
    PixelFormat expected_input;
    PixelFormat produced;
    int opencv_code;
};

class ColorConvertTool final : public VisionTool {
public:
    ColorConvertTool()
        : VisionTool("nvision.tool.processing.color-convert", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const std::string conversion =
            context.parameter(ParameterKey("conversion")).enumeration_value().value();

        Conversion selected{};
        if (conversion == "bayer-gr-to-bgr") {
            selected = {PixelFormat::bayer_gr8, PixelFormat::bgr8, cv::COLOR_BayerGR2BGR};
        } else if (conversion == "bgr-to-gray") {
            selected = {PixelFormat::bgr8, PixelFormat::mono8, cv::COLOR_BGR2GRAY};
        } else if (conversion == "bgr-to-hsv") {
            selected = {PixelFormat::bgr8, PixelFormat::hsv8, cv::COLOR_BGR2HSV};
        } else if (conversion == "gray-to-bgr") {
            selected = {PixelFormat::mono8, PixelFormat::bgr8, cv::COLOR_GRAY2BGR};
        } else {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_CONVERSION", "conversion " + conversion + " is not implemented"));
        }

        if (image.pixel_format() != selected.expected_input) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "conversion " + conversion + " does not accept the pixel format of its input"));
        }

        cv::Mat converted;
        cv::cvtColor(interop::borrow(image), converted, selected.opencv_code);

        PortValueMap outputs;
        outputs.set(
            PortKey("image"),
            produced_image(context, PortKey("image"), converted, selected.produced));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class ChannelExtractTool final : public VisionTool {
public:
    ChannelExtractTool()
        : VisionTool("nvision.tool.processing.channel-extract", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (!is_three_channel(image.pixel_format())) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "channel extract requires a three-channel image"));
        }

        const auto channel = static_cast<int>(
            context.parameter(ParameterKey("channel")).integer_value());
        if (channel > 2) {
            return ToolExecutionOutcome::failed(configuration_error(
                "CHANNEL_OUT_OF_RANGE",
                "the input image has three channels, so channel " + std::to_string(channel) +
                    " does not exist"));
        }

        cv::Mat extracted;
        cv::extractChannel(interop::borrow(image), extracted, channel);

        PortValueMap outputs;
        outputs.set(
            PortKey("image"),
            produced_image(context, PortKey("image"), extracted, PixelFormat::mono8));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_color_convert_tool() {
    return std::make_shared<const ColorConvertTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_channel_extract_tool() {
    return std::make_shared<const ChannelExtractTool>();
}

}  // namespace nvision::vision
