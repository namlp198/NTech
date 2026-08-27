#include "threshold_tool.hpp"

#include "opencv_interop.hpp"

#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/execution_result.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/port_value.hpp"

#include <opencv2/imgproc.hpp>

#include <string>
#include <utility>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::ErrorCategory;
using core::ExecutionError;
using core::ParameterKey;
using core::PortKey;
using core::ResultCode;
using core::SemanticVersion;
using core::ToolTypeId;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;
using workflow::ToolImplementation;

ExecutionError configuration_error(std::string code, std::string detail) {
    return ExecutionError(
        ResultCode(std::move(code)), ErrorCategory::configuration, std::move(detail), false);
}

bool is_single_channel(const PixelFormat format) noexcept {
    return format == PixelFormat::mono8 || format == PixelFormat::mono16;
}

class ThresholdTool final : public ToolImplementation {
public:
    ThresholdTool() : id_("nvision.tool.processing.threshold"), version_(1U, 0U, 0U) {}

    [[nodiscard]] const ToolTypeId& tool_type_id() const noexcept override {
        return id_;
    }

    [[nodiscard]] const SemanticVersion& tool_version() const noexcept override {
        return version_;
    }

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (!is_single_channel(image.pixel_format())) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "threshold requires a single-channel image; convert colour first"));
        }

        const std::string mode = context.parameter(ParameterKey("mode")).enumeration_value().value();
        int operation = 0;
        if (mode == "binary") {
            operation = cv::THRESH_BINARY;
        } else if (mode == "binary-inverted") {
            operation = cv::THRESH_BINARY_INV;
        } else {
            // The catalog also declares truncate and to-zero. Both keep source intensity,
            // so their result is an image rather than a mask, and this tool declares a
            // mask output. Refusing is honest; the definition needs splitting or a
            // version bump before those modes can be offered.
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_THRESHOLD_MODE",
                "threshold mode " + mode + " produces intensity rather than a mask"));
        }

        const double value = context.parameter(ParameterKey("value")).floating_point_value();

        const cv::Mat source = interop::borrow(image);
        cv::Mat mask;
        // A mask is single-channel 8-bit with 0 excluded and 255 included, whatever the
        // depth of the image it was derived from.
        cv::threshold(source, mask, value, 255.0, operation);
        if (mask.type() != CV_8UC1) {
            mask.convertTo(mask, CV_8UC1);
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("mask"),
            PortValue::mask(ImageView(
                interop::materialize(context.output_image_id(PortKey("mask")), mask))));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }

private:
    ToolTypeId id_;
    SemanticVersion version_;
};

}  // namespace

std::shared_ptr<const ToolImplementation> create_threshold_tool() {
    return std::make_shared<const ThresholdTool>();
}

}  // namespace nvision::vision
