#pragma once

#include "nvision/core/identifier.hpp"

#include <array>
#include <vector>

namespace nvision::core {

class Point2D final {
public:
    Point2D(CoordinateSpaceId coordinate_space, double x, double y);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] double x() const noexcept;
    [[nodiscard]] double y() const noexcept;

    [[nodiscard]] bool operator==(const Point2D& other) const noexcept = default;

private:
    CoordinateSpaceId coordinate_space_;
    double x_;
    double y_;
};

class Line2D final {
public:
    Line2D(Point2D start, Point2D end);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const Point2D& start() const noexcept;
    [[nodiscard]] const Point2D& end() const noexcept;

private:
    Point2D start_;
    Point2D end_;
};

class Circle final {
public:
    Circle(Point2D center, double radius);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const Point2D& center() const noexcept;
    [[nodiscard]] double radius() const noexcept;

private:
    Point2D center_;
    double radius_;
};

class AxisAlignedRectangle final {
public:
    AxisAlignedRectangle(
        CoordinateSpaceId coordinate_space,
        double x,
        double y,
        double width,
        double height);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] double x() const noexcept;
    [[nodiscard]] double y() const noexcept;
    [[nodiscard]] double width() const noexcept;
    [[nodiscard]] double height() const noexcept;

private:
    CoordinateSpaceId coordinate_space_;
    double x_;
    double y_;
    double width_;
    double height_;
};

class Polygon final {
public:
    explicit Polygon(std::vector<Point2D> vertices);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const std::vector<Point2D>& vertices() const noexcept;

private:
    std::vector<Point2D> vertices_;
};

class Transform2D final {
public:
    Transform2D(
        CoordinateSpaceId source_space,
        CoordinateSpaceId target_space,
        std::array<double, 9> matrix);

    [[nodiscard]] const CoordinateSpaceId& source_space() const noexcept;
    [[nodiscard]] const CoordinateSpaceId& target_space() const noexcept;
    [[nodiscard]] const std::array<double, 9>& matrix() const noexcept;
    [[nodiscard]] Point2D apply(const Point2D& point) const;

private:
    CoordinateSpaceId source_space_;
    CoordinateSpaceId target_space_;
    std::array<double, 9> matrix_;
};

}  // namespace nvision::core
