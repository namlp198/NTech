#include "nvision/core/overlay.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace nvision::core {

namespace {

constexpr std::size_t maximum_label_length = 256U;

Point2D first_point_of(const OverlayShape& shape) {
    return std::visit(
        [](const auto& value) -> Point2D {
            using Shape = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Shape, Point2D>) {
                return value;
            } else if constexpr (std::is_same_v<Shape, Line2D>) {
                return value.start();
            } else if constexpr (std::is_same_v<Shape, Circle>) {
                return value.center();
            } else if constexpr (std::is_same_v<Shape, Polygon>) {
                return value.vertices().front();
            } else if constexpr (std::is_same_v<Shape, Contour>) {
                return value.points().front();
            } else {
                return Point2D(value.coordinate_space(), value.x(), value.y());
            }
        },
        shape);
}

}  // namespace

OverlayStyle::OverlayStyle(
    const std::uint8_t red,
    const std::uint8_t green,
    const std::uint8_t blue,
    const double thickness,
    const bool filled)
    : red_(red), green_(green), blue_(blue), thickness_(thickness), filled_(filled) {
    if (!std::isfinite(thickness_) || thickness_ <= 0.0) {
        throw std::invalid_argument("an overlay stroke thickness must be finite and positive");
    }
}

std::uint8_t OverlayStyle::red() const noexcept {
    return red_;
}

std::uint8_t OverlayStyle::green() const noexcept {
    return green_;
}

std::uint8_t OverlayStyle::blue() const noexcept {
    return blue_;
}

double OverlayStyle::thickness() const noexcept {
    return thickness_;
}

bool OverlayStyle::filled() const noexcept {
    return filled_;
}

OverlayItem::OverlayItem(OverlayShape shape, OverlayStyle style, std::string label)
    : shape_(std::move(shape)), style_(std::move(style)), label_(std::move(label)) {
    if (label_.size() > maximum_label_length) {
        throw std::invalid_argument("an overlay label exceeds its maximum length");
    }
    if (std::any_of(label_.begin(), label_.end(), [](const unsigned char character) {
            return character < 0x20U;
        })) {
        throw std::invalid_argument("an overlay label contains a control character");
    }
}

const OverlayShape& OverlayItem::shape() const noexcept {
    return shape_;
}

const OverlayStyle& OverlayItem::style() const noexcept {
    return style_;
}

const std::string& OverlayItem::label() const noexcept {
    return label_;
}

const CoordinateSpaceId& OverlayItem::coordinate_space() const {
    return std::visit(
        [](const auto& value) -> const CoordinateSpaceId& { return value.coordinate_space(); },
        shape_);
}

Point2D OverlayItem::anchor() const {
    return first_point_of(shape_);
}

Overlay::Overlay(CoordinateSpaceId coordinate_space, std::vector<OverlayItem> items)
    : coordinate_space_(std::move(coordinate_space)), items_(std::move(items)) {
    for (const OverlayItem& item : items_) {
        if (item.coordinate_space() != coordinate_space_) {
            throw std::invalid_argument(
                "every overlay shape must be in the coordinate space of the overlay");
        }
    }
}

const CoordinateSpaceId& Overlay::coordinate_space() const noexcept {
    return coordinate_space_;
}

const std::vector<OverlayItem>& Overlay::items() const noexcept {
    return items_;
}

std::size_t Overlay::size() const noexcept {
    return items_.size();
}

}  // namespace nvision::core
