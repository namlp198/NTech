#include "tool_support.hpp"

#include "nvision/acquisition/image_view.hpp"

#include <utility>

namespace nvision::vision {

using acquisition::PixelFormat;

const core::SemanticVersion& initial_tool_version() {
    static const core::SemanticVersion version(1U, 0U, 0U);
    return version;
}

VisionTool::VisionTool(std::string id, core::SemanticVersion version)
    : id_(std::move(id)), version_(std::move(version)) {}

const core::ToolTypeId& VisionTool::tool_type_id() const noexcept {
    return id_;
}

const core::SemanticVersion& VisionTool::tool_version() const noexcept {
    return version_;
}

core::ExecutionError configuration_error(std::string code, std::string detail) {
    return core::ExecutionError(
        core::ResultCode(std::move(code)),
        core::ErrorCategory::configuration,
        std::move(detail),
        false);
}

bool is_single_channel(const PixelFormat format) noexcept {
    return format == PixelFormat::mono8 || format == PixelFormat::mono16 ||
           format == PixelFormat::bayer_gr8 || format == PixelFormat::mono_float32;
}

bool is_three_channel(const PixelFormat format) noexcept {
    return format == PixelFormat::rgb8 || format == PixelFormat::bgr8 ||
           format == PixelFormat::hsv8;
}

workflow::PortValue produced_image(
    const workflow::ToolExecutionContext& context,
    const core::PortKey& output_port,
    const cv::Mat& produced,
    const PixelFormat format) {
    return workflow::PortValue::image(acquisition::ImageView(
        interop::materialize(context.output_image_id(output_port), produced, format)));
}

workflow::PortValue produced_mask(
    const workflow::ToolExecutionContext& context,
    const core::PortKey& output_port,
    const cv::Mat& produced) {
    return workflow::PortValue::mask(acquisition::ImageView(interop::materialize(
        context.output_image_id(output_port), produced, PixelFormat::mono8)));
}

int odd_size_parameter(
    const workflow::ToolExecutionContext& context,
    const core::ParameterKey& key) {
    const auto value = static_cast<int>(context.parameter(key).integer_value());
    return value % 2 == 0 ? 0 : value;
}

}  // namespace nvision::vision
