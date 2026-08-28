#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_histogram_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_sharpness_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_contrast_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_edge_density_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_ncc_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_ssim_tool();

}  // namespace nvision::vision
