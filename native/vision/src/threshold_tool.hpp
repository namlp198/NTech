#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

// Threshold turns a single-channel image into a mask. It is the first tool whose result
// is new pixels rather than a window onto existing ones, so it is also the proof that the
// computed-image ownership contract works end to end.
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_threshold_tool();

}  // namespace nvision::vision
