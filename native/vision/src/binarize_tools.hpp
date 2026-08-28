#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_adaptive_threshold_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_color_range_tool();

}  // namespace nvision::vision
