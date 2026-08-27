#include "nvision/vision/builtin_tool_implementations.hpp"

#include "crop_tool.hpp"

namespace nvision::vision {

void register_vision_tool_implementations(workflow::ToolImplementationRegistry& registry) {
    registry.register_implementation(create_crop_tool());
}

workflow::ToolImplementationRegistry create_vision_implementation_registry() {
    workflow::ToolImplementationRegistry registry;
    register_vision_tool_implementations(registry);
    return registry;
}

}  // namespace nvision::vision
