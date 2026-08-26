#pragma once

#include <cstdint>

namespace nvision::core {

enum class PortDataType : std::uint32_t {
    frame,
    image,
    image_view,
    mask,
    region,
    point_2d,
    point_list,
    line_2d,
    circle,
    contour,
    contour_list,
    bounding_box,
    polygon,
    transform,
    homography,
    feature_set,
    match_set,
    detection,
    detection_list,
    text_result,
    text_result_list,
    barcode_result,
    measurement,
    measurement_list,
    defect,
    defect_list,
    point_grid_model,
    point_grid_result,
    boolean,
    integer,
    floating_point,
    string,
    decision,
    evidence,
    overlay,
    any_metadata,
};

}  // namespace nvision::core
