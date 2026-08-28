#pragma once

#include "nvision/workflow/tool_implementation_registry.hpp"

namespace nvision::vision {

// Registers the generic vision tool implementations this milestone provides.
//
// Registration is deliberately partial. A catalog definition without an
// implementation binds as not_implemented, which is how remaining M3 coverage stays
// measurable instead of surfacing as a missing tool.
void register_vision_tool_implementations(workflow::ToolImplementationRegistry& registry);

[[nodiscard]] workflow::ToolImplementationRegistry create_vision_implementation_registry();

}  // namespace nvision::vision
