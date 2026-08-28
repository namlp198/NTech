#include "nvision/acquisition/image.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace nvision::acquisition {

Image::Image(
    core::ImageId id,
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint32_t stride_bytes,
    const PixelFormat pixel_format,
    std::vector<std::uint8_t> bytes)
    : id_(std::move(id)),
      width_(width),
      height_(height),
      stride_bytes_(stride_bytes),
      pixel_format_(pixel_format),
      bytes_(std::move(bytes)) {
    if (id_.value().size() > maximum_image_identity_length) {
        throw std::invalid_argument("image identity is too long to derive a coordinate space");
    }

    if (width_ == 0U || height_ == 0U) {
        throw std::invalid_argument("image dimensions must be positive");
    }

    const std::size_t pixel_size = bytes_per_pixel(pixel_format_);
    if (width_ > std::numeric_limits<std::size_t>::max() / pixel_size) {
        throw std::overflow_error("image row size overflows addressable memory");
    }

    const std::size_t minimum_stride = static_cast<std::size_t>(width_) * pixel_size;
    if (stride_bytes_ < minimum_stride) {
        throw std::invalid_argument("image stride is smaller than one packed row");
    }

    const std::size_t stride = stride_bytes_;
    if (height_ > std::numeric_limits<std::size_t>::max() / stride) {
        throw std::overflow_error("image buffer size overflows addressable memory");
    }

    // A computed image has no partial state to describe, so an undersized buffer is
    // always a producer defect rather than a delivery condition.
    if (bytes_.size() < stride * height_) {
        throw std::invalid_argument("image buffer is smaller than its declared dimensions");
    }
}

const core::ImageId& Image::id() const noexcept {
    return id_;
}

std::uint32_t Image::width() const noexcept {
    return width_;
}

std::uint32_t Image::height() const noexcept {
    return height_;
}

std::uint32_t Image::stride_bytes() const noexcept {
    return stride_bytes_;
}

PixelFormat Image::pixel_format() const noexcept {
    return pixel_format_;
}

std::span<const std::uint8_t> Image::bytes() const noexcept {
    return bytes_;
}

}  // namespace nvision::acquisition
