#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_k_means_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_dbscan_tool();
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_mean_shift_tool();

}  // namespace nvision::vision
