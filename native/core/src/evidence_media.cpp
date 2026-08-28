#include "nvision/core/evidence_media.hpp"

#include <stdexcept>

namespace nvision::core {

namespace {

bool is_valid(const EvidenceEncoding encoding) noexcept {
    return encoding == EvidenceEncoding::raw_packed || encoding == EvidenceEncoding::png ||
           encoding == EvidenceEncoding::jpeg || encoding == EvidenceEncoding::tiff;
}

}  // namespace

std::uint32_t channel_count_of(const EvidenceChannelLayout layout) {
    switch (layout) {
        // An undecoded sensor pattern is one component per pixel. What those components
        // mean together is the decoder's business, not the storage layout's.
        case EvidenceChannelLayout::mono:
        case EvidenceChannelLayout::bayer_gr:
            return 1U;
        case EvidenceChannelLayout::rgb:
        case EvidenceChannelLayout::bgr:
        case EvidenceChannelLayout::hsv:
            return 3U;
        case EvidenceChannelLayout::bgra:
            return 4U;
    }
    throw std::invalid_argument("evidence channel layout is unsupported");
}

std::uint32_t byte_count_of(const EvidenceComponentType component) {
    switch (component) {
        case EvidenceComponentType::uint8:
            return 1U;
        case EvidenceComponentType::uint16:
            return 2U;
        case EvidenceComponentType::float32:
            return 4U;
    }
    throw std::invalid_argument("evidence component type is unsupported");
}

EvidenceMedia::EvidenceMedia(
    const std::uint32_t width,
    const std::uint32_t height,
    const EvidenceEncoding encoding,
    const EvidenceChannelLayout channel_layout,
    const EvidenceComponentType component_type)
    : width_(width),
      height_(height),
      encoding_(encoding),
      channel_layout_(channel_layout),
      component_type_(component_type) {
    if (width_ == 0U || height_ == 0U) {
        throw std::invalid_argument("evidence media must have a positive width and height");
    }
    if (!is_valid(encoding_)) {
        throw std::invalid_argument("evidence encoding is unsupported");
    }
    // Both throw on an unsupported value, so constructing the descriptor is also what
    // validates the layout and component it declares.
    static_cast<void>(channel_count_of(channel_layout_));
    static_cast<void>(byte_count_of(component_type_));
}

std::uint32_t EvidenceMedia::width() const noexcept {
    return width_;
}

std::uint32_t EvidenceMedia::height() const noexcept {
    return height_;
}

EvidenceEncoding EvidenceMedia::encoding() const noexcept {
    return encoding_;
}

EvidenceChannelLayout EvidenceMedia::channel_layout() const noexcept {
    return channel_layout_;
}

EvidenceComponentType EvidenceMedia::component_type() const noexcept {
    return component_type_;
}

std::uint32_t EvidenceMedia::channel_count() const noexcept {
    return channel_count_of(channel_layout_);
}

std::string EvidenceMedia::media_type() const {
    switch (encoding_) {
        case EvidenceEncoding::png:
            return "image/png";
        case EvidenceEncoding::jpeg:
            return "image/jpeg";
        case EvidenceEncoding::tiff:
            return "image/tiff";
        case EvidenceEncoding::raw_packed:
        default:
            // Packed pixels are not a picture any reader can open on their own, so they are
            // named as the bytes they are and described by the rest of this contract.
            return "application/octet-stream";
    }
}

std::uint64_t EvidenceMedia::packed_length_bytes() const noexcept {
    if (encoding_ != EvidenceEncoding::raw_packed) {
        return 0U;
    }
    return static_cast<std::uint64_t>(width_) * static_cast<std::uint64_t>(height_) *
           static_cast<std::uint64_t>(channel_count_of(channel_layout_)) *
           static_cast<std::uint64_t>(byte_count_of(component_type_));
}

}  // namespace nvision::core
