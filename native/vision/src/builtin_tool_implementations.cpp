#include "nvision/vision/builtin_tool_implementations.hpp"

#include "binarize_tools.hpp"
#include "color_tools.hpp"
#include "crop_tool.hpp"
#include "filter_tools.hpp"
#include "morphology_tools.hpp"
#include "threshold_tool.hpp"
#include "warp_tools.hpp"

namespace nvision::vision {

void register_vision_tool_implementations(workflow::ToolImplementationRegistry& registry) {
    // Image Processing.
    registry.register_implementation(create_color_convert_tool());
    registry.register_implementation(create_channel_extract_tool());
    registry.register_implementation(create_crop_tool());
    registry.register_implementation(create_resize_tool());
    registry.register_implementation(create_normalize_tool());
    registry.register_implementation(create_clahe_tool());
    registry.register_implementation(create_gaussian_blur_tool());
    registry.register_implementation(create_sharpen_tool());
    registry.register_implementation(create_threshold_tool());
    registry.register_implementation(create_adaptive_threshold_tool());
    registry.register_implementation(create_color_range_tool());
    registry.register_implementation(create_morphology_tool());
    registry.register_implementation(create_distance_transform_tool());
    registry.register_implementation(create_perspective_warp_tool());
    registry.register_implementation(create_affine_transform_tool());
}

workflow::ToolImplementationRegistry create_vision_implementation_registry() {
    workflow::ToolImplementationRegistry registry;
    register_vision_tool_implementations(registry);
    return registry;
}

}  // namespace nvision::vision
