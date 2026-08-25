#pragma once

#include "nvision/acquisition/frame.hpp"
#include "nvision/core/geometry.hpp"

#include <cstdint>
#include <span>

namespace nvision::acquisition {

class ImageView final {
public:
    explicit ImageView(FrameLease frame);
    ImageView(
        FrameLease frame,
        std::uint32_t origin_x,
        std::uint32_t origin_y,
        std::uint32_t width,
        std::uint32_t height);

    [[nodiscard]] const FrameLease& frame() const noexcept;
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
    FrameLease frame_;
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
