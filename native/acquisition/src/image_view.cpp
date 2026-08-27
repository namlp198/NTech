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

const Image& require_image(const ImageLease& image) {
    if (!image) {
        throw std::invalid_argument("image view requires an image lease");
    }
    return *image;
}

std::string frame_identity(const Frame& frame) {
    std::ostringstream identity;
    identity << std::hex << std::setfill('0');
    for (const std::uint8_t value : frame.id()) {
        identity << std::setw(2) << static_cast<unsigned int>(value);
    }
    return identity.str();
}

core::CoordinateSpaceId make_source_space(const std::string& prefix) {
    return core::CoordinateSpaceId(prefix + ".image-pixel");
}

// A subview names itself by its absolute position in the source raster, never by
// appending to its parent, so nesting cannot grow the identifier without bound.
core::CoordinateSpaceId make_view_space(
    const ImageSource& source,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height) {
    if (origin_x == 0U && origin_y == 0U && width == source.width() &&
        height == source.height()) {
        return make_source_space(source.space_prefix());
    }

    return core::CoordinateSpaceId(
        source.space_prefix() + ".view." + std::to_string(origin_x) + "_" +
        std::to_string(origin_y) + "_" + std::to_string(width) + "_" +
        std::to_string(height));
}

std::array<double, 9> translation(const double x, const double y) noexcept {
    return std::array<double, 9>{1.0, 0.0, x, 0.0, 1.0, y, 0.0, 0.0, 1.0};
}

void validate_bounds(
    const ImageSource& source,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height) {
    if (width == 0U || height == 0U) {
        throw std::invalid_argument("image view dimensions must be positive");
    }
    if (origin_x >= source.width() || origin_y >= source.height() ||
        width > source.width() - origin_x || height > source.height() - origin_y) {
        throw std::out_of_range("image view lies outside the source raster");
    }
}

}  // namespace

ImageSource::ImageSource(FrameLease frame) : raster_(frame) {
    const Frame& raster = require_frame(frame);
    if (std::all_of(raster.id().begin(), raster.id().end(), [](const std::uint8_t value) {
            return value == 0U;
        })) {
        throw std::invalid_argument("image view requires a non-empty frame identifier");
    }
    if (raster.status() != FrameStatus::complete) {
        throw std::invalid_argument("image view requires a complete frame");
    }
    space_prefix_ = "nvision.frame." + frame_identity(raster);
}

ImageSource::ImageSource(ImageLease image) : raster_(image) {
    space_prefix_ = "nvision.image." + require_image(image).id().value();
}

bool ImageSource::is_captured() const noexcept {
    return std::holds_alternative<FrameLease>(raster_);
}

FrameLease ImageSource::frame() const noexcept {
    const FrameLease* lease = std::get_if<FrameLease>(&raster_);
    return lease == nullptr ? FrameLease{} : *lease;
}

ImageLease ImageSource::image() const noexcept {
    const ImageLease* lease = std::get_if<ImageLease>(&raster_);
    return lease == nullptr ? ImageLease{} : *lease;
}

std::uint32_t ImageSource::width() const noexcept {
    return is_captured() ? std::get<FrameLease>(raster_)->width()
                         : std::get<ImageLease>(raster_)->width();
}

std::uint32_t ImageSource::height() const noexcept {
    return is_captured() ? std::get<FrameLease>(raster_)->height()
                         : std::get<ImageLease>(raster_)->height();
}

std::uint32_t ImageSource::stride_bytes() const noexcept {
    return is_captured() ? std::get<FrameLease>(raster_)->stride_bytes()
                         : std::get<ImageLease>(raster_)->stride_bytes();
}

PixelFormat ImageSource::pixel_format() const noexcept {
    return is_captured() ? std::get<FrameLease>(raster_)->pixel_format()
                         : std::get<ImageLease>(raster_)->pixel_format();
}

std::span<const std::uint8_t> ImageSource::bytes() const noexcept {
    return is_captured() ? std::get<FrameLease>(raster_)->buffer()->bytes()
                         : std::get<ImageLease>(raster_)->bytes();
}

const std::string& ImageSource::space_prefix() const noexcept {
    return space_prefix_;
}

ImageView::ImageView(FrameLease frame) : ImageView(ImageSource(std::move(frame))) {}

ImageView::ImageView(ImageLease image) : ImageView(ImageSource(std::move(image))) {}

ImageView::ImageView(ImageSource source)
    : ImageView(source, 0U, 0U, source.width(), source.height()) {}

ImageView::ImageView(
    FrameLease frame,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height)
    : ImageView(ImageSource(std::move(frame)), origin_x, origin_y, width, height) {}

ImageView::ImageView(
    ImageSource source,
    const std::uint32_t origin_x,
    const std::uint32_t origin_y,
    const std::uint32_t width,
    const std::uint32_t height)
    : source_(std::move(source)),
      origin_x_(origin_x),
      origin_y_(origin_y),
      width_(width),
      height_(height),
      source_coordinate_space_(make_source_space(source_.space_prefix())),
      coordinate_space_(make_view_space(source_, origin_x, origin_y, width, height)),
      view_to_source_(
          coordinate_space_,
          source_coordinate_space_,
          translation(static_cast<double>(origin_x), static_cast<double>(origin_y))),
      source_to_view_(
          source_coordinate_space_,
          coordinate_space_,
          translation(-static_cast<double>(origin_x), -static_cast<double>(origin_y))) {
    validate_bounds(source_, origin_x_, origin_y_, width_, height_);
}

const ImageSource& ImageView::source() const noexcept {
    return source_;
}

FrameLease ImageView::frame() const noexcept {
    return source_.frame();
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
    return source_.stride_bytes();
}

PixelFormat ImageView::pixel_format() const noexcept {
    return source_.pixel_format();
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

    const std::size_t pixel_size = bytes_per_pixel(source_.pixel_format());
    const std::size_t offset =
        static_cast<std::size_t>(origin_y_ + row_index) * source_.stride_bytes() +
        static_cast<std::size_t>(origin_x_) * pixel_size;
    const std::size_t length = static_cast<std::size_t>(width_) * pixel_size;
    return source_.bytes().subspan(offset, length);
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

    return ImageView(source_, origin_x_ + origin_x, origin_y_ + origin_y, width, height);
}

}  // namespace nvision::acquisition
