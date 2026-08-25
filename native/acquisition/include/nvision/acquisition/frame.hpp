#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace nvision::acquisition {

enum class PixelFormat : std::uint32_t {
    mono8 = 0,
    mono16 = 1,
    rgb8 = 2,
    bgr8 = 3,
};

enum class FrameStatus : std::uint32_t {
    complete = 0,
    incomplete = 1,
    corrupt = 2,
    dropped = 3,
    timeout = 4,
    cancelled = 5,
};

enum class TimestampQuality : std::uint32_t {
    host_only = 0,
    camera_unsynchronized = 1,
    camera_synchronized = 2,
    ptp_synchronized = 3,
    external_clock = 4,
};

[[nodiscard]] std::size_t bytes_per_pixel(PixelFormat format);

class FrameBuffer final {
public:
    explicit FrameBuffer(std::vector<std::uint8_t> bytes);

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;

    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;

private:
    const std::vector<std::uint8_t> bytes_;
};

struct FrameMetadata final {
    std::string source_file;
    std::string source_path;
    std::string frame_role;
};

class Frame final {
public:
    Frame(
        std::array<std::uint8_t, 16> id,
        std::string camera_id,
        std::uint64_t sequence_number,
        std::int64_t captured_at_unix_ns,
        std::int64_t received_at_unix_ns,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t stride_bytes,
        PixelFormat pixel_format,
        std::shared_ptr<const FrameBuffer> buffer,
        FrameStatus status,
        TimestampQuality timestamp_quality,
        FrameMetadata metadata);

    [[nodiscard]] const std::array<std::uint8_t, 16>& id() const noexcept;
    [[nodiscard]] const std::string& camera_id() const noexcept;
    [[nodiscard]] std::uint64_t sequence_number() const noexcept;
    [[nodiscard]] std::int64_t captured_at_unix_ns() const noexcept;
    [[nodiscard]] std::int64_t received_at_unix_ns() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t stride_bytes() const noexcept;
    [[nodiscard]] PixelFormat pixel_format() const noexcept;
    [[nodiscard]] FrameStatus status() const noexcept;
    [[nodiscard]] TimestampQuality timestamp_quality() const noexcept;
    [[nodiscard]] const std::shared_ptr<const FrameBuffer>& buffer() const noexcept;
    [[nodiscard]] const FrameMetadata& metadata() const noexcept;
    [[nodiscard]] std::string metadata_json() const;

private:
    const std::array<std::uint8_t, 16> id_;
    const std::string camera_id_;
    const std::uint64_t sequence_number_;
    const std::int64_t captured_at_unix_ns_;
    const std::int64_t received_at_unix_ns_;
    const std::uint32_t width_;
    const std::uint32_t height_;
    const std::uint32_t stride_bytes_;
    const PixelFormat pixel_format_;
    const std::shared_ptr<const FrameBuffer> buffer_;
    const FrameStatus status_;
    const TimestampQuality timestamp_quality_;
    const FrameMetadata metadata_;
};

using FrameLease = std::shared_ptr<const Frame>;

}  // namespace nvision::acquisition
