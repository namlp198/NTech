#include "nvision/core/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace nvision::core {

namespace {

void require_finite(const double value, const char* field) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string(field) + " must be finite");
    }
}

double determinant(const std::array<double, 9>& matrix) noexcept {
    return matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7]) -
           matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6]) +
           matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]);
}

double maximum_absolute_value(const std::array<double, 9>& values) noexcept {
    double maximum = 0.0;
    for (const double value : values) {
        maximum = std::max(maximum, std::abs(value));
    }
    return maximum;
}

}  // namespace

Point2D::Point2D(CoordinateSpaceId coordinate_space, const double x, const double y)
    : coordinate_space_(std::move(coordinate_space)), x_(x), y_(y) {
    require_finite(x_, "x");
    require_finite(y_, "y");
}

const CoordinateSpaceId& Point2D::coordinate_space() const noexcept {
    return coordinate_space_;
}

double Point2D::x() const noexcept {
    return x_;
}

double Point2D::y() const noexcept {
    return y_;
}

Line2D::Line2D(Point2D start, Point2D end)
    : start_(std::move(start)), end_(std::move(end)) {
    if (start_.coordinate_space() != end_.coordinate_space()) {
        throw std::invalid_argument("line endpoints must use the same coordinate space");
    }
    if (start_ == end_) {
        throw std::invalid_argument("line endpoints must be distinct");
    }
}

const CoordinateSpaceId& Line2D::coordinate_space() const noexcept {
    return start_.coordinate_space();
}

const Point2D& Line2D::start() const noexcept {
    return start_;
}

const Point2D& Line2D::end() const noexcept {
    return end_;
}

Circle::Circle(Point2D center, const double radius)
    : center_(std::move(center)), radius_(radius) {
    require_finite(radius_, "radius");
    if (radius_ <= 0.0) {
        throw std::invalid_argument("radius must be positive");
    }
}

const CoordinateSpaceId& Circle::coordinate_space() const noexcept {
    return center_.coordinate_space();
}

const Point2D& Circle::center() const noexcept {
    return center_;
}

double Circle::radius() const noexcept {
    return radius_;
}

AxisAlignedRectangle::AxisAlignedRectangle(
    CoordinateSpaceId coordinate_space,
    const double x,
    const double y,
    const double width,
    const double height)
    : coordinate_space_(std::move(coordinate_space)),
      x_(x),
      y_(y),
      width_(width),
      height_(height) {
    require_finite(x_, "x");
    require_finite(y_, "y");
    require_finite(width_, "width");
    require_finite(height_, "height");
    if (width_ <= 0.0 || height_ <= 0.0) {
        throw std::invalid_argument("rectangle width and height must be positive");
    }
}

const CoordinateSpaceId& AxisAlignedRectangle::coordinate_space() const noexcept {
    return coordinate_space_;
}

double AxisAlignedRectangle::x() const noexcept {
    return x_;
}

double AxisAlignedRectangle::y() const noexcept {
    return y_;
}

double AxisAlignedRectangle::width() const noexcept {
    return width_;
}

double AxisAlignedRectangle::height() const noexcept {
    return height_;
}

Polygon::Polygon(std::vector<Point2D> vertices) : vertices_(std::move(vertices)) {
    if (vertices_.size() < 3U) {
        throw std::invalid_argument("polygon must contain at least three vertices");
    }

    const CoordinateSpaceId& space = vertices_.front().coordinate_space();
    if (!std::all_of(vertices_.begin(), vertices_.end(), [&space](const Point2D& point) {
            return point.coordinate_space() == space;
        })) {
        throw std::invalid_argument("polygon vertices must use the same coordinate space");
    }

    double twice_signed_area = 0.0;
    double coordinate_scale = 0.0;
    for (std::size_t index = 0U; index < vertices_.size(); ++index) {
        const Point2D& current = vertices_[index];
        const Point2D& next = vertices_[(index + 1U) % vertices_.size()];
        twice_signed_area += current.x() * next.y() - next.x() * current.y();
        coordinate_scale = std::max(
            coordinate_scale,
            std::max(std::abs(current.x()), std::abs(current.y())));
    }
    const double area_scale = std::max(1.0, coordinate_scale * coordinate_scale);
    if (std::abs(twice_signed_area) <= 1.0e-12 * area_scale) {
        throw std::invalid_argument("polygon must enclose a non-zero area");
    }
}

const CoordinateSpaceId& Polygon::coordinate_space() const noexcept {
    return vertices_.front().coordinate_space();
}

const std::vector<Point2D>& Polygon::vertices() const noexcept {
    return vertices_;
}

Contour::Contour(
    std::vector<Point2D> points,
    const bool closed,
    std::optional<std::size_t> parent)
    : points_(std::move(points)), closed_(closed), parent_(std::move(parent)) {
    if (points_.empty()) {
        throw std::invalid_argument("contour must contain at least one point");
    }

    const CoordinateSpaceId& space = points_.front().coordinate_space();
    if (!std::all_of(points_.begin(), points_.end(), [&space](const Point2D& point) {
            return point.coordinate_space() == space;
        })) {
        throw std::invalid_argument("contour points must use the same coordinate space");
    }
}

const CoordinateSpaceId& Contour::coordinate_space() const noexcept {
    return points_.front().coordinate_space();
}

const std::vector<Point2D>& Contour::points() const noexcept {
    return points_;
}

bool Contour::closed() const noexcept {
    return closed_;
}

const std::optional<std::size_t>& Contour::parent() const noexcept {
    return parent_;
}

Transform2D::Transform2D(
    CoordinateSpaceId source_space,
    CoordinateSpaceId target_space,
    std::array<double, 9> matrix)
    : source_space_(std::move(source_space)),
      target_space_(std::move(target_space)),
      matrix_(std::move(matrix)) {
    if (!std::all_of(matrix_.begin(), matrix_.end(), [](const double value) {
            return std::isfinite(value);
        })) {
        throw std::invalid_argument("transform matrix values must be finite");
    }
    const double matrix_scale = maximum_absolute_value(matrix_);
    std::array<double, 9> normalized_matrix = matrix_;
    if (matrix_scale > 0.0) {
        for (double& value : normalized_matrix) {
            value /= matrix_scale;
        }
    }
    if (matrix_scale == 0.0 || std::abs(determinant(normalized_matrix)) <= 1.0e-12) {
        throw std::invalid_argument("transform matrix must be invertible");
    }
}

const CoordinateSpaceId& Transform2D::source_space() const noexcept {
    return source_space_;
}

const CoordinateSpaceId& Transform2D::target_space() const noexcept {
    return target_space_;
}

const std::array<double, 9>& Transform2D::matrix() const noexcept {
    return matrix_;
}

Point2D Transform2D::apply(const Point2D& point) const {
    if (point.coordinate_space() != source_space_) {
        throw std::invalid_argument("point coordinate space does not match transform source");
    }

    const double homogeneous_x = matrix_[0] * point.x() + matrix_[1] * point.y() + matrix_[2];
    const double homogeneous_y = matrix_[3] * point.x() + matrix_[4] * point.y() + matrix_[5];
    const double homogeneous_w = matrix_[6] * point.x() + matrix_[7] * point.y() + matrix_[8];
    const double homogeneous_scale = std::max(
        std::abs(homogeneous_x),
        std::max(std::abs(homogeneous_y), std::abs(homogeneous_w)));
    if (!std::isfinite(homogeneous_scale) || homogeneous_scale == 0.0 ||
        std::abs(homogeneous_w) <= 1.0e-12 * homogeneous_scale) {
        throw std::invalid_argument("transform maps point to an invalid homogeneous coordinate");
    }

    return Point2D(target_space_, homogeneous_x / homogeneous_w, homogeneous_y / homogeneous_w);
}

}  // namespace nvision::core
