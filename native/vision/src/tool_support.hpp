#pragma once

// Internal to nvision_vision. Shared plumbing so each tool implementation contains its
// algorithm and its contract checks, and nothing else.

#include "opencv_interop.hpp"

#include "nvision/core/execution_result.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/semantic_version.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/port_value.hpp"
#include "nvision/workflow/tool_execution.hpp"

#include <cstdint>
#include <string>

namespace nvision::vision {

// Every catalog definition is at 1.0.0. A tool that has to change its contract bumps its
// own version rather than sharing this one.
[[nodiscard]] const core::SemanticVersion& initial_tool_version();

// A tool whose identity is fixed at construction, which is every built-in tool.
class VisionTool : public workflow::ToolImplementation {
public:
    VisionTool(std::string id, core::SemanticVersion version);

    [[nodiscard]] const core::ToolTypeId& tool_type_id() const noexcept final;
    [[nodiscard]] const core::SemanticVersion& tool_version() const noexcept final;

private:
    core::ToolTypeId id_;
    core::SemanticVersion version_;
};

// A tool refusing work it cannot do correctly, as opposed to a fault while doing it.
[[nodiscard]] core::ExecutionError configuration_error(std::string code, std::string detail);

[[nodiscard]] bool is_single_channel(acquisition::PixelFormat format) noexcept;
[[nodiscard]] bool is_three_channel(acquisition::PixelFormat format) noexcept;

// Wraps an OpenCV result as an image on one of the tool's output ports, giving it the
// identity the execution context derives for that port.
[[nodiscard]] workflow::PortValue produced_image(
    const workflow::ToolExecutionContext& context,
    const core::PortKey& output_port,
    const cv::Mat& produced,
    acquisition::PixelFormat format);

// The same for a mask port. A mask is single-channel 8-bit, 0 excluded and 255 included.
[[nodiscard]] workflow::PortValue produced_mask(
    const workflow::ToolExecutionContext& context,
    const core::PortKey& output_port,
    const cv::Mat& produced);

// Reads an integer parameter that OpenCV requires to be an odd size, such as a kernel or
// a neighbourhood block. Returns 0 when the value is even, so the caller can refuse.
[[nodiscard]] int odd_size_parameter(
    const workflow::ToolExecutionContext& context,
    const core::ParameterKey& key);

}  // namespace nvision::vision
