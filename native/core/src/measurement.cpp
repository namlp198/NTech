#include "nvision/core/measurement.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace nvision::core {

namespace {

bool is_valid(const MeasurementUnit unit) noexcept {
    return unit == MeasurementUnit::none || unit == MeasurementUnit::pixel ||
           unit == MeasurementUnit::square_pixel || unit == MeasurementUnit::ratio ||
           unit == MeasurementUnit::count || unit == MeasurementUnit::degree;
}

bool is_spatial_unit(const MeasurementUnit unit) noexcept {
    return unit == MeasurementUnit::pixel || unit == MeasurementUnit::square_pixel;
}

}  // namespace

Measurement::Measurement(
    MeasurementKey key,
    const double value,
    const MeasurementUnit unit,
    std::optional<CoordinateSpaceId> coordinate_space)
    : key_(std::move(key)),
      value_(value),
      unit_(unit),
      coordinate_space_(std::move(coordinate_space)) {
    if (!is_valid(unit_)) {
        throw std::invalid_argument("measurement unit is unsupported");
    }
    if (!std::isfinite(value_)) {
        throw std::invalid_argument("measurement value must be finite");
    }
    // A pixel distance is meaningless without the raster it was measured in, and a pure
    // number has no space to name, so the two must agree.
    if (is_spatial_unit(unit_) != coordinate_space_.has_value()) {
        throw std::invalid_argument(
            "a measurement in image units must declare its coordinate space, and only one "
            "in image units may declare it");
    }
    if (unit_ == MeasurementUnit::count && (value_ < 0.0 || std::floor(value_) != value_)) {
        throw std::invalid_argument("a count measurement must be a non-negative whole number");
    }
    if (unit_ == MeasurementUnit::square_pixel && value_ < 0.0) {
        throw std::invalid_argument("an area measurement must not be negative");
    }
}

const MeasurementKey& Measurement::key() const noexcept {
    return key_;
}

double Measurement::value() const noexcept {
    return value_;
}

MeasurementUnit Measurement::unit() const noexcept {
    return unit_;
}

const std::optional<CoordinateSpaceId>& Measurement::coordinate_space() const noexcept {
    return coordinate_space_;
}

bool Measurement::is_spatial() const noexcept {
    return is_spatial_unit(unit_);
}

}  // namespace nvision::core
