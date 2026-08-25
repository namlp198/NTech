#include "nvision/acquisition/image_view.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace nvision::acquisition {

namespace {

const Frame& require_frame(const FrameLease& frame) {
    if (!frame) {
        throw std::invalid_argument("image view requires a frame lease");
    }
    return *frame;
}

std::string frame_identity(const Frame& frame) {
    std::ostringstream identity;
    identity << std::hex << std::setfill('0');
    for (const std::uint8_t value : frame.id()) {
        identity << std::setw(2) << static_cast<unsigned int>(value);
    }
    return identity.str();
}

core::CoordinateSpaceId make_source_space(const Frame& frame) {
    return core::CoordinateSpaceId("nvision.frame." + frame_identity(frame) + ".image-pixel");
}

core::CoordinateSpaceId make_view_space(
    const Frame& frame,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height) {
    if (origin_x == 0U && origin_y == 0U && width == frame.width() &&
        height == frame.height()) {
        return make_source_space(frame);
    }

    return core::CoordinateSpaceId(
        "nvision.frame." + frame_identity(frame) + ".view." + std::to_string(origin_x) +
        "_" + std::to_string(origin_y) + "_" + std::to_string(width) + "_" +
        std::to_string(height));
}

std::array<double, 9> translation(const double x, const double y) noexcept {
    return std::array<double, 9>{1.0, 0.0, x, 0.0, 1.0, y, 0.0, 0.0, 1.0};
}

void validate_bounds(
    const Frame& frame,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height) {
    if (std::all_of(frame.id().begin(), frame.id().end(), [](const std::uint8_t value) {
            return value == 0U;
        })) {
        throw std::invalid_argument("image view requires a non-empty frame identifier");
    }
    if (frame.status() != FrameStatus::complete) {
        throw std::invalid_argument("image view requires a complete frame");
    }
    if (width == 0U || height == 0U) {
        throw std::invalid_argument("image view dimensions must be positive");
    }
    if (origin_x >= frame.width() || origin_y >= frame.height() ||
        width > frame.width() - origin_x || height > frame.height() - origin_y) {
        throw std::out_of_range("image view lies outside the source frame");
    }
}

}  // namespace

ImageView::ImageView(FrameLease frame)
    : ImageView(
          frame,
          0U,
          0U,
          require_frame(frame).width(),
          require_frame(frame).height()) {}

ImageView::ImageView(
    FrameLease frame,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height)
    : frame_(std::move(frame)),
      origin_x_(origin_x),
      origin_y_(origin_y),
      width_(width),
      height_(height),
      source_coordinate_space_(make_source_space(require_frame(frame_))),
      coordinate_space_(make_view_space(require_frame(frame_), origin_x, origin_y, width, height)),
      view_to_source_(
          coordinate_space_,
          source_coordinate_space_,
          translation(static_cast<double>(origin_x), static_cast<double>(origin_y))),
      source_to_view_(
          source_coordinate_space_,
          coordinate_space_,
          translation(-static_cast<double>(origin_x), -static_cast<double>(origin_y))) {
    validate_bounds(*frame_, origin_x_, origin_y_, width_, height_);
}

const FrameLease& ImageView::frame() const noexcept {
    return frame_;
}

std::uint32_t ImageView::origin_x() const noexcept {
    return origin_x_;
}

std::uint32_t ImageView::origin_y() const noexcept {
    return origin_y_;
}

std::uint32_t ImageView::width() const noexcept {
    return width_;
}

std::uint32_t ImageView::height() const noexcept {
    return height_;
}

std::uint32_t ImageView::stride_bytes() const noexcept {
    return frame_->stride_bytes();
}

PixelFormat ImageView::pixel_format() const noexcept {
    return frame_->pixel_format();
}

const core::CoordinateSpaceId& ImageView::source_coordinate_space() const noexcept {
    return source_coordinate_space_;
}

const core::CoordinateSpaceId& ImageView::coordinate_space() const noexcept {
    return coordinate_space_;
}

const core::Transform2D& ImageView::view_to_source() const noexcept {
    return view_to_source_;
}

const core::Transform2D& ImageView::source_to_view() const noexcept {
    return source_to_view_;
}

std::span<const std::uint8_t> ImageView::row(const std::uint32_t row_index) const {
    if (row_index >= height_) {
        throw std::out_of_range("image view row is outside the view");
    }

    const std::size_t pixel_size = bytes_per_pixel(frame_->pixel_format());
    const std::size_t offset =
        static_cast<std::size_t>(origin_y_ + row_index) * frame_->stride_bytes() +
        static_cast<std::size_t>(origin_x_) * pixel_size;
    const std::size_t length = static_cast<std::size_t>(width_) * pixel_size;
    return frame_->buffer()->bytes().subspan(offset, length);
}

ImageView ImageView::subview(
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height) const {
    if (width == 0U || height == 0U || origin_x >= width_ || origin_y >= height_ ||
        width > width_ - origin_x || height > height_ - origin_y) {
        throw std::out_of_range("subview lies outside the parent image view");
    }

    return ImageView(frame_, origin_x_ + origin_x, origin_y_ + origin_y, width, height);
}

}  // namespace nvision::acquisition
