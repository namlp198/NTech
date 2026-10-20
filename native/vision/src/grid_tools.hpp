#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation>
create_sort_points_along_line_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_point_grid_learn_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_grid_deviation_tool();

}  // namespace nvision::vision
