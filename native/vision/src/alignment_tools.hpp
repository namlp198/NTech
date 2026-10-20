#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_line_fit_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation>
create_feature_detect_and_match_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation>
create_homography_estimate_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation>
create_homography_validate_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_ecc_refine_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_point_transform_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_point_assignment_tool();

}  // namespace nvision::vision
