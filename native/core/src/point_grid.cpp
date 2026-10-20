#include "nvision/core/point_grid.hpp"

#include <stdexcept>
#include <utility>

namespace nvision::core {

PointGridModel::PointGridModel(
    CoordinateSpaceId coordinate_space,
    std::vector<std::vector<Point2D>> rows)
    : coordinate_space_(std::move(coordinate_space)), rows_(std::move(rows)) {
    if (rows_.empty()) {
        throw std::invalid_argument("a point grid model must contain at least one row");
    }
    for (const std::vector<Point2D>& row : rows_) {
        if (row.empty()) {
            throw std::invalid_argument("a point grid row must contain at least one position");
        }
        for (const Point2D& point : row) {
            if (point.coordinate_space() != coordinate_space_) {
                throw std::invalid_argument(
                    "every grid position must be in the coordinate space of the model");
            }
        }
    }
}

const CoordinateSpaceId& PointGridModel::coordinate_space() const noexcept {
    return coordinate_space_;
}

const std::vector<std::vector<Point2D>>& PointGridModel::rows() const noexcept {
    return rows_;
}

std::size_t PointGridModel::row_count() const noexcept {
    return rows_.size();
}

std::size_t PointGridModel::point_count() const noexcept {
    std::size_t total = 0U;
    for (const std::vector<Point2D>& row : rows_) {
        total += row.size();
    }
    return total;
}

std::vector<Point2D> PointGridModel::points() const {
    std::vector<Point2D> flattened;
    flattened.reserve(point_count());
    for (const std::vector<Point2D>& row : rows_) {
        flattened.insert(flattened.end(), row.begin(), row.end());
    }
    return flattened;
}

}  // namespace nvision::core
