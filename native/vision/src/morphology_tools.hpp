#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_morphology_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_distance_transform_tool();

}  // namespace nvision::vision
