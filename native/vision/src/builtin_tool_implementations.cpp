#include "nvision/vision/builtin_tool_implementations.hpp"

#include "alignment_tools.hpp"
#include "binarize_tools.hpp"
#include "cluster_tools.hpp"
#include "color_tools.hpp"
#include "count_tools.hpp"
#include "crop_tool.hpp"
#include "evidence_tools.hpp"
#include "filter_tools.hpp"
#include "grid_tools.hpp"
#include "morphology_tools.hpp"
#include "quality_tools.hpp"
#include "region_tools.hpp"
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

    // Location and Geometry.
    registry.register_implementation(create_connected_components_tool());
    registry.register_implementation(create_contour_extraction_tool());
    registry.register_implementation(create_blob_analysis_tool());
    registry.register_implementation(create_minimum_enclosing_circle_tool());
    registry.register_implementation(create_line_fit_tool());
    registry.register_implementation(create_feature_detect_and_match_tool());
    registry.register_implementation(create_homography_estimate_tool());
    registry.register_implementation(create_homography_validate_tool());
    registry.register_implementation(create_ecc_refine_tool());
    registry.register_implementation(create_point_transform_tool());
    registry.register_implementation(create_point_assignment_tool());

    // Measurement. Grid deviation is registered with the point-grid tools below, because
    // the model it measures against is defined there.
    registry.register_implementation(create_pixel_count_tool());
    registry.register_implementation(create_object_count_tool());
    registry.register_implementation(create_distance_tool());
    registry.register_implementation(create_radius_diameter_tool());
    registry.register_implementation(create_histogram_tool());
    registry.register_implementation(create_sharpness_tool());
    registry.register_implementation(create_contrast_tool());
    registry.register_implementation(create_edge_density_tool());
    registry.register_implementation(create_ncc_tool());
    registry.register_implementation(create_ssim_tool());

    // Clustering and Structure. Point grid inspect is absent on purpose: it receives its
    // model as a versioned resource rather than through a port, so it needs the resource
    // binding that arrives with the workflow execution milestone.
    registry.register_implementation(create_k_means_tool());
    registry.register_implementation(create_dbscan_tool());
    registry.register_implementation(create_mean_shift_tool());
    registry.register_implementation(create_sort_points_along_line_tool());
    registry.register_implementation(create_point_grid_learn_tool());
    registry.register_implementation(create_grid_deviation_tool());

    // Logic and Results. Only evidence output belongs to the vision milestone; the rest of
    // the category is workflow logic that arrives with job and recipe execution.
    registry.register_implementation(create_evidence_output_tool());
}

workflow::ToolImplementationRegistry create_vision_implementation_registry() {
    workflow::ToolImplementationRegistry registry;
    register_vision_tool_implementations(registry);
    return registry;
}

}  // namespace nvision::vision
