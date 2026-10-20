#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_pixel_count_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_object_count_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_distance_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_radius_diameter_tool();

}  // namespace nvision::vision
