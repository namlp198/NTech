#include "filter_tools.hpp"

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

// A filter keeps the pixel format it was given: it changes intensities, not meaning.
ToolExecutionOutcome succeeded_with(
    const ToolExecutionContext& context,
    const cv::Mat& produced,
    const PixelFormat format) {
    PortValueMap outputs;
    outputs.set(PortKey("image"), produced_image(context, PortKey("image"), produced, format));
    return ToolExecutionOutcome::succeeded(std::move(outputs));
}

class GaussianBlurTool final : public VisionTool {
public:
    GaussianBlurTool()
        : VisionTool("nvision.tool.processing.gaussian-blur", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const int kernel = odd_size_parameter(context, ParameterKey("kernel-size"));
        if (kernel == 0) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_KERNEL_SIZE", "a Gaussian kernel size must be odd"));
        }

        const double sigma = context.parameter(ParameterKey("sigma")).floating_point_value();
        cv::Mat blurred;
        // A zero sigma asks OpenCV to derive it from the kernel size, which is the
        // conventional meaning of the parameter rather than no blurring at all.
        cv::GaussianBlur(interop::borrow(image), blurred, cv::Size(kernel, kernel), sigma, sigma);
        return succeeded_with(context, blurred, image.pixel_format());
    }
};

class SharpenTool final : public VisionTool {
public:
    SharpenTool() : VisionTool("nvision.tool.processing.sharpen", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const double strength = context.parameter(ParameterKey("strength")).floating_point_value();

        // Unsharp masking: add back the difference between the image and a blurred copy,
        // so strength 0 is an exact copy and larger values amplify local contrast.
        const cv::Mat source = interop::borrow(image);
        cv::Mat blurred;
        cv::GaussianBlur(source, blurred, cv::Size(3, 3), 0.0, 0.0);
        cv::Mat sharpened;
        cv::addWeighted(source, 1.0 + strength, blurred, -strength, 0.0, sharpened);
        return succeeded_with(context, sharpened, image.pixel_format());
    }
};

class ClaheTool final : public VisionTool {
public:
    ClaheTool() : VisionTool("nvision.tool.processing.clahe", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (image.pixel_format() != PixelFormat::mono8 &&
            image.pixel_format() != PixelFormat::mono16) {
            // OpenCV equalises one channel at a time, and choosing which channel of a
            // colour image to equalise is a decision the graph must make explicitly.
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "CLAHE requires an 8-bit or 16-bit single-channel image"));
        }

        const double clip_limit =
            context.parameter(ParameterKey("clip-limit")).floating_point_value();
        const auto tile = static_cast<int>(
            context.parameter(ParameterKey("tile-size")).integer_value());

        cv::Mat equalized;
        cv::createCLAHE(clip_limit, cv::Size(tile, tile))->apply(interop::borrow(image), equalized);
        return succeeded_with(context, equalized, image.pixel_format());
    }
};

class NormalizeTool final : public VisionTool {
public:
    NormalizeTool() : VisionTool("nvision.tool.processing.normalize", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const double minimum = context.parameter(ParameterKey("minimum")).floating_point_value();
        const double maximum = context.parameter(ParameterKey("maximum")).floating_point_value();
        if (minimum >= maximum) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_RANGE", "the normalize minimum must be below its maximum"));
        }

        cv::Mat normalized;
        cv::normalize(
            interop::borrow(image),
            normalized,
            minimum,
            maximum,
            cv::NORM_MINMAX,
            interop::opencv_type_of(image.pixel_format()));
        return succeeded_with(context, normalized, image.pixel_format());
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_gaussian_blur_tool() {
    return std::make_shared<const GaussianBlurTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_sharpen_tool() {
    return std::make_shared<const SharpenTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_clahe_tool() {
    return std::make_shared<const ClaheTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_normalize_tool() {
    return std::make_shared<const NormalizeTool>();
}

}  // namespace nvision::vision
