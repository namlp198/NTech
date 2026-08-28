#pragma once

#include "nvision/acquisition/frame.hpp"
#include "nvision/acquisition/image.hpp"
#include "nvision/core/geometry.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <variant>

namespace nvision::acquisition {

// Where the pixels an ImageView exposes come from. A view never owns pixels; it keeps
// the lease of whichever raster does, so the bytes outlive every view over them.
class ImageSource final {
public:
    explicit ImageSource(FrameLease frame);
    explicit ImageSource(ImageLease image);

    [[nodiscard]] bool is_captured() const noexcept;
    // The captured frame these pixels came from, or a null lease for a computed image.
    [[nodiscard]] FrameLease frame() const noexcept;
    // The computed image these pixels came from, or a null lease for a captured frame.
    [[nodiscard]] ImageLease image() const noexcept;

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t stride_bytes() const noexcept;
    [[nodiscard]] PixelFormat pixel_format() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;
    // The stable identity a coordinate space for this raster is derived from.
    [[nodiscard]] const std::string& space_prefix() const noexcept;

private:
    std::variant<FrameLease, ImageLease> raster_;
    std::string space_prefix_;
};

class ImageView final {
public:
    explicit ImageView(FrameLease frame);
    explicit ImageView(ImageLease image);
    explicit ImageView(ImageSource source);
    ImageView(
        FrameLease frame,
        std::uint32_t origin_x,
        std::uint32_t origin_y,
        std::uint32_t width,
        std::uint32_t height);
    ImageView(
        ImageSource source,
        std::uint32_t origin_x,
        std::uint32_t origin_y,
        std::uint32_t width,
        std::uint32_t height);

    [[nodiscard]] const ImageSource& source() const noexcept;
    [[nodiscard]] FrameLease frame() const noexcept;
    [[nodiscard]] std::uint32_t origin_x() const noexcept;
    [[nodiscard]] std::uint32_t origin_y() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t stride_bytes() const noexcept;
    [[nodiscard]] PixelFormat pixel_format() const noexcept;
    [[nodiscard]] const core::CoordinateSpaceId& source_coordinate_space() const noexcept;
    [[nodiscard]] const core::CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const core::Transform2D& view_to_source() const noexcept;
    [[nodiscard]] const core::Transform2D& source_to_view() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> row(std::uint32_t row_index) const;
    [[nodiscard]] ImageView subview(
        std::uint32_t origin_x,
        std::uint32_t origin_y,
        std::uint32_t width,
        std::uint32_t height) const;

private:
    ImageSource source_;
    std::uint32_t origin_x_;
    std::uint32_t origin_y_;
    std::uint32_t width_;
    std::uint32_t height_;
    core::CoordinateSpaceId source_coordinate_space_;
    core::CoordinateSpaceId coordinate_space_;
    core::Transform2D view_to_source_;
    core::Transform2D source_to_view_;
};

}  // namespace nvision::acquisition
