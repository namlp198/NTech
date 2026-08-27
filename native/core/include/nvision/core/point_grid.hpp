#pragma once

#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"

#include <cstddef>
#include <vector>

namespace nvision::core {

// A learned layout of expected point positions, arranged in rows.
//
// The model records where features are expected to be and nothing about what they mean.
// Whether a position holds a pin, a solder pad or a fiducial is the project's business,
// and tolerances belong to the recipe that inspects against the model rather than to the
// model itself, so the same layout can be inspected strictly or loosely.
class PointGridModel final {
public:
    PointGridModel(CoordinateSpaceId coordinate_space, std::vector<std::vector<Point2D>> rows);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const std::vector<std::vector<Point2D>>& rows() const noexcept;
    [[nodiscard]] std::size_t row_count() const noexcept;
    [[nodiscard]] std::size_t point_count() const noexcept;
    // Every expected position in row order, then column order within each row.
    [[nodiscard]] std::vector<Point2D> points() const;

private:
    CoordinateSpaceId coordinate_space_;
    std::vector<std::vector<Point2D>> rows_;
};

}  // namespace nvision::core
