#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

// Evidence Output renders an optional overlay over an image and records the result as an
// immutable, content-addressed artifact.
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_evidence_output_tool();

}  // namespace nvision::vision
