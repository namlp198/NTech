#pragma once

#include "nvision/core/identifier.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace nvision::core {

// What a measured number means. A measurement without a unit is not comparable, and a
// number whose unit is only implied cannot be turned into a calibrated result later.
enum class MeasurementUnit : std::uint32_t {
    // A pure number: a determinant, a residual, an index.
    none = 0,
    pixel = 1,
    square_pixel = 2,
    // A dimensionless fraction, such as an inlier ratio or a correlation score.
    ratio = 3,
    count = 4,
    degree = 5,
};

// One named number a tool measured.
//
// A measurement expressed in image units also names the coordinate space it was measured
// in, because the same distance means different things in a full frame and in a resized
// or warped view of it.
class Measurement final {
public:
    Measurement(
        MeasurementKey key,
        double value,
        MeasurementUnit unit,
        std::optional<CoordinateSpaceId> coordinate_space = std::nullopt);

    [[nodiscard]] const MeasurementKey& key() const noexcept;
    [[nodiscard]] double value() const noexcept;
    [[nodiscard]] MeasurementUnit unit() const noexcept;
    [[nodiscard]] const std::optional<CoordinateSpaceId>& coordinate_space() const noexcept;
    [[nodiscard]] bool is_spatial() const noexcept;

private:
    MeasurementKey key_;
    double value_;
    MeasurementUnit unit_;
    std::optional<CoordinateSpaceId> coordinate_space_;
};

// Measurements a tool produced together, keeping the order the tool reported them in so a
// repeated run yields an identical result.
using MeasurementList = std::vector<Measurement>;

}  // namespace nvision::core
