#pragma once

#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"

#include <optional>
#include <vector>

namespace nvision::core {

// A region a tool located, whatever found it: a connected component, a blob, a classifier
// box. It says where the region is and how large it is, and leaves what the region means
// to the project that configured the graph.
class Detection final {
public:
    Detection(
        AxisAlignedRectangle bounds,
        Point2D centroid,
        double area,
        std::optional<double> score = std::nullopt,
        std::optional<DetectionLabel> label = std::nullopt);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const AxisAlignedRectangle& bounds() const noexcept;
    [[nodiscard]] const Point2D& centroid() const noexcept;
    // The selected area in square pixels, which is not the area of the bounding box.
    [[nodiscard]] double area() const noexcept;
    [[nodiscard]] const std::optional<double>& score() const noexcept;
    [[nodiscard]] const std::optional<DetectionLabel>& label() const noexcept;

private:
    AxisAlignedRectangle bounds_;
    Point2D centroid_;
    double area_;
    std::optional<double> score_;
    std::optional<DetectionLabel> label_;
};

using DetectionList = std::vector<Detection>;

}  // namespace nvision::core
