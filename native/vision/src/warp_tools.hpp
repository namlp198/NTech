#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_resize_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_perspective_warp_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_affine_transform_tool();

}  // namespace nvision::vision
