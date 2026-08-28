#include "threshold_tool.hpp"

#include "tool_support.hpp"

#include <opencv2/imgproc.hpp>

#include <string>
#include <utility>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using core::ParameterKey;
using core::PortKey;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

class ThresholdTool final : public VisionTool {
public:
    ThresholdTool()
        : VisionTool("nvision.tool.processing.threshold", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (!is_single_channel(image.pixel_format())) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "threshold requires a single-channel image; convert colour first"));
        }

        // The definition offers only the two modes that produce a mask, so the schema has
        // already rejected anything else before the tool runs.
        const std::string mode =
            context.parameter(ParameterKey("mode")).enumeration_value().value();
        const int operation = mode == "binary-inverted" ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;

        const double value = context.parameter(ParameterKey("value")).floating_point_value();

        cv::Mat mask;
        // A mask is single-channel 8-bit with 0 excluded and 255 included, whatever the
        // depth of the image it was derived from.
        cv::threshold(interop::borrow(image), mask, value, 255.0, operation);
        if (mask.type() != CV_8UC1) {
            mask.convertTo(mask, CV_8UC1);
        }

        PortValueMap outputs;
        outputs.set(PortKey("mask"), produced_mask(context, PortKey("mask"), mask));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_threshold_tool() {
    return std::make_shared<const ThresholdTool>();
}

}  // namespace nvision::vision
