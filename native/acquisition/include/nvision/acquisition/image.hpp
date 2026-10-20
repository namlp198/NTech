#pragma once

#include "nvision/acquisition/frame.hpp"
#include "nvision/core/identifier.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace nvision::acquisition {

// The longest image identity that still leaves room for a derived coordinate space.
inline constexpr std::size_t maximum_image_identity_length = 64U;

// A raster a tool produced, as opposed to a Frame, which a camera captured.
//
// An Image owns its pixels and deliberately carries no acquisition metadata. It has no
// camera, no sequence number, and no capture timestamps, because inventing those would
// make a computed raster indistinguishable from a real capture in results and evidence.
// Its identity is supplied by the producer and must be stable, so a repeated run derives
// the same coordinate space for the same output.
class Image final {
public:
    Image(
        core::ImageId id,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t stride_bytes,
        PixelFormat pixel_format,
        std::vector<std::uint8_t> bytes);

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;

    [[nodiscard]] const core::ImageId& id() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t stride_bytes() const noexcept;
    [[nodiscard]] PixelFormat pixel_format() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;

private:
    core::ImageId id_;
    std::uint32_t width_;
    std::uint32_t height_;
    std::uint32_t stride_bytes_;
    PixelFormat pixel_format_;
    std::vector<std::uint8_t> bytes_;
};

using ImageLease = std::shared_ptr<const Image>;

}  // namespace nvision::acquisition
