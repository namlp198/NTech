#pragma once

#include "nvision/core/tool_registry.hpp"

namespace nvision::core {

// Registers the initial generic built-in tool tree.
//
// Every definition here must stay application neutral. A concrete inspection is
// expressed as a graph of these tools plus Recipe configuration, never as a built-in
// tool that carries product vocabulary. Parameter sets are the minimum the reference
// graphs need; the vision and recognition milestones refine them alongside the
// implementations.
void register_builtin_tools(ToolRegistry& registry);

[[nodiscard]] ToolRegistry create_builtin_registry();

}  // namespace nvision::core
