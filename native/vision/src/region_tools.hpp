#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_connected_components_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_contour_extraction_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_blob_analysis_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation>
create_minimum_enclosing_circle_tool();

}  // namespace nvision::vision
