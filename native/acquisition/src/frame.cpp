#include "nvision/acquisition/frame.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nvision::acquisition {

namespace {

std::string escape_json(std::string_view value) {
    std::ostringstream escaped;
    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            escaped << "\\\"";
            break;
        case '\\':
            escaped << "\\\\";
            break;
        case '\b':
            escaped << "\\b";
            break;
        case '\f':
            escaped << "\\f";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            if (character < 0x20U) {
                constexpr char digits[] = "0123456789abcdef";
                escaped << "\\u00" << digits[(character >> 4U) & 0x0FU]
                        << digits[character & 0x0FU];
            } else {
                escaped << static_cast<char>(character);
            }
        }
    }

    return escaped.str();
}

}  // namespace

std::size_t bytes_per_pixel(const PixelFormat format) {
    switch (format) {
    case PixelFormat::mono8:
        return 1U;
    case PixelFormat::mono16:
        return 2U;
    case PixelFormat::bayer_gr8:
        return 1U;
    case PixelFormat::rgb8:
    case PixelFormat::bgr8:
    case PixelFormat::hsv8:
        return 3U;
    case PixelFormat::mono_float32:
        return 4U;
    }

    throw std::invalid_argument("pixel format is unsupported");
}

FrameBuffer::FrameBuffer(std::vector<std::uint8_t> bytes)
    : bytes_(std::move(bytes)) {}

std::span<const std::uint8_t> FrameBuffer::bytes() const noexcept {
    return bytes_;
}

Frame::Frame(
    std::array<std::uint8_t, 16> id,
    std::string camera_id,
    const std::uint64_t sequence_number,
    const std::int64_t captured_at_unix_ns,
    const std::int64_t received_at_unix_ns,
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint32_t stride_bytes,
    const PixelFormat pixel_format,
    std::shared_ptr<const FrameBuffer> buffer,
    const FrameStatus status,
    const TimestampQuality timestamp_quality,
    FrameMetadata metadata)
    : id_(id),
      camera_id_(std::move(camera_id)),
      sequence_number_(sequence_number),
      captured_at_unix_ns_(captured_at_unix_ns),
      received_at_unix_ns_(received_at_unix_ns),
      width_(width),
      height_(height),
      stride_bytes_(stride_bytes),
      pixel_format_(pixel_format),
      buffer_(std::move(buffer)),
      status_(status),
      timestamp_quality_(timestamp_quality),
      metadata_(std::move(metadata)) {
    if (camera_id_.empty()) {
        throw std::invalid_argument("camera identifier is required");
    }

    if (received_at_unix_ns_ < captured_at_unix_ns_) {
        throw std::invalid_argument("received timestamp precedes captured timestamp");
    }

    if (width_ == 0U || height_ == 0U) {
        throw std::invalid_argument("frame dimensions must be positive");
    }

    const std::size_t pixel_size = bytes_per_pixel(pixel_format_);
    if (width_ > std::numeric_limits<std::size_t>::max() / pixel_size) {
        throw std::overflow_error("frame row size overflows addressable memory");
    }

    const std::size_t minimum_stride = static_cast<std::size_t>(width_) * pixel_size;
    if (stride_bytes_ < minimum_stride) {
        throw std::invalid_argument("frame stride is smaller than one packed row");
    }

    if (!buffer_) {
        throw std::invalid_argument("frame buffer is required");
    }

    const std::size_t stride = stride_bytes_;
    if (height_ > std::numeric_limits<std::size_t>::max() / stride) {
        throw std::overflow_error("frame buffer size overflows addressable memory");
    }

    const std::size_t required_length = stride * height_;
    if (status_ == FrameStatus::complete && buffer_->bytes().size() < required_length) {
        throw std::invalid_argument("complete frame buffer is smaller than its declared dimensions");
    }
}

const std::array<std::uint8_t, 16>& Frame::id() const noexcept {
    return id_;
}

const std::string& Frame::camera_id() const noexcept {
    return camera_id_;
}

std::uint64_t Frame::sequence_number() const noexcept {
    return sequence_number_;
}

std::int64_t Frame::captured_at_unix_ns() const noexcept {
    return captured_at_unix_ns_;
}

std::int64_t Frame::received_at_unix_ns() const noexcept {
    return received_at_unix_ns_;
}

std::uint32_t Frame::width() const noexcept {
    return width_;
}

std::uint32_t Frame::height() const noexcept {
    return height_;
}

std::uint32_t Frame::stride_bytes() const noexcept {
    return stride_bytes_;
}

PixelFormat Frame::pixel_format() const noexcept {
    return pixel_format_;
}

FrameStatus Frame::status() const noexcept {
    return status_;
}

TimestampQuality Frame::timestamp_quality() const noexcept {
    return timestamp_quality_;
}

const std::shared_ptr<const FrameBuffer>& Frame::buffer() const noexcept {
    return buffer_;
}

const FrameMetadata& Frame::metadata() const noexcept {
    return metadata_;
}

std::string Frame::metadata_json() const {
    return "{\"sourceFile\":\"" + escape_json(metadata_.source_file) +
           "\",\"sourcePath\":\"" + escape_json(metadata_.source_path) +
           "\",\"frameRole\":\"" + escape_json(metadata_.frame_role) + "\"}";
}

}  // namespace nvision::acquisition
