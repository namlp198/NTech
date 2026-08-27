#pragma once

#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace nvision::core {

// How one overlay shape should be drawn. Style is carried as data so that a result is
// never the side effect of something having been drawn: the same overlay renders into an
// evidence image, a Studio canvas, or a Runtime preview without the engine knowing which.
class OverlayStyle final {
public:
    OverlayStyle(
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue,
        double thickness = 1.0,
        bool filled = false);

    [[nodiscard]] std::uint8_t red() const noexcept;
    [[nodiscard]] std::uint8_t green() const noexcept;
    [[nodiscard]] std::uint8_t blue() const noexcept;
    [[nodiscard]] double thickness() const noexcept;
    [[nodiscard]] bool filled() const noexcept;

private:
    std::uint8_t red_;
    std::uint8_t green_;
    std::uint8_t blue_;
    double thickness_;
    bool filled_;
};

using OverlayShape =
    std::variant<Point2D, Line2D, Circle, AxisAlignedRectangle, Polygon, Contour>;

// One annotated shape. The label is plain text a renderer places near the shape; it is
// never the place to put a result a caller should be reading from a typed port.
class OverlayItem final {
public:
    OverlayItem(OverlayShape shape, OverlayStyle style, std::string label = {});

    [[nodiscard]] const OverlayShape& shape() const noexcept;
    [[nodiscard]] const OverlayStyle& style() const noexcept;
    [[nodiscard]] const std::string& label() const noexcept;
    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const;
    // The point a renderer anchors this item's label to.
    [[nodiscard]] Point2D anchor() const;

private:
    OverlayShape shape_;
    OverlayStyle style_;
    std::string label_;
};

// Shapes a tool wants drawn over one raster, all in that raster's coordinate space.
class Overlay final {
public:
    Overlay(CoordinateSpaceId coordinate_space, std::vector<OverlayItem> items);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const std::vector<OverlayItem>& items() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    CoordinateSpaceId coordinate_space_;
    std::vector<OverlayItem> items_;
};

}  // namespace nvision::core
