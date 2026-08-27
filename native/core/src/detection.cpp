#include "nvision/core/detection.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace nvision::core {

Detection::Detection(
    AxisAlignedRectangle bounds,
    Point2D centroid,
    const double area,
    std::optional<double> score,
    std::optional<DetectionLabel> label)
    : bounds_(std::move(bounds)),
      centroid_(std::move(centroid)),
      area_(area),
      score_(std::move(score)),
      label_(std::move(label)) {
    if (centroid_.coordinate_space() != bounds_.coordinate_space()) {
        throw std::invalid_argument(
            "a detection centroid must be in the same coordinate space as its bounds");
    }
    if (!std::isfinite(area_) || area_ < 0.0) {
        throw std::invalid_argument("a detection area must be finite and non-negative");
    }
    // A selected region cannot cover more than the box that encloses it.
    if (area_ > bounds_.width() * bounds_.height()) {
        throw std::invalid_argument("a detection area cannot exceed the area of its bounds");
    }
    if (score_.has_value() && !std::isfinite(*score_)) {
        throw std::invalid_argument("a detection score must be finite");
    }
}

const CoordinateSpaceId& Detection::coordinate_space() const noexcept {
    return bounds_.coordinate_space();
}

const AxisAlignedRectangle& Detection::bounds() const noexcept {
    return bounds_;
}

const Point2D& Detection::centroid() const noexcept {
    return centroid_;
}

double Detection::area() const noexcept {
    return area_;
}

const std::optional<double>& Detection::score() const noexcept {
    return score_;
}

const std::optional<DetectionLabel>& Detection::label() const noexcept {
    return label_;
}

}  // namespace nvision::core
