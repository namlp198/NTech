#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_gaussian_blur_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_sharpen_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_clahe_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_normalize_tool();

}  // namespace nvision::vision
