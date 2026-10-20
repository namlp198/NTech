#pragma once

#include <cstdint>
#include <string>

namespace nvision::core {

// How the stored bytes of an evidence artifact are arranged. `raw_packed` means the pixels
// themselves, row after row with no padding, so a reader needs the rest of this descriptor
// to make sense of them. The container formats already describe themselves.
enum class EvidenceEncoding : std::uint32_t {
    raw_packed = 0,
    png = 1,
    jpeg = 2,
    tiff = 3,
};

// What one pixel holds, and in which order. This is a media vocabulary rather than the
// acquisition one: it says what a consumer will find in the bytes. A sensor pattern or a
// colour space no image reader understands is still recorded truthfully instead of being
// passed off as something a viewer would misinterpret.
enum class EvidenceChannelLayout : std::uint32_t {
    mono = 0,
    rgb = 1,
    bgr = 2,
    bgra = 3,
    hsv = 4,
    bayer_gr = 5,
};

enum class EvidenceComponentType : std::uint32_t {
    uint8 = 0,
    uint16 = 1,
    float32 = 2,
};

[[nodiscard]] std::uint32_t channel_count_of(EvidenceChannelLayout layout);
[[nodiscard]] std::uint32_t byte_count_of(EvidenceComponentType component);

// Enough for a consumer to read an evidence artifact without asking the engine what it
// wrote. Everything here describes the stored bytes rather than the raster they came from,
// because the raster is gone by the time anyone reads the evidence.
class EvidenceMedia final {
public:
    EvidenceMedia(
        std::uint32_t width,
        std::uint32_t height,
        EvidenceEncoding encoding,
        EvidenceChannelLayout channel_layout,
        EvidenceComponentType component_type);

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] EvidenceEncoding encoding() const noexcept;
    [[nodiscard]] EvidenceChannelLayout channel_layout() const noexcept;
    [[nodiscard]] EvidenceComponentType component_type() const noexcept;
    [[nodiscard]] std::uint32_t channel_count() const noexcept;

    // Derived rather than stored, so a declared media type can never disagree with the
    // encoding it is supposed to name.
    [[nodiscard]] std::string media_type() const;

    // The exact number of bytes packed pixels occupy. A container format compresses, so
    // its stored length is not predictable and this reports zero for one.
    [[nodiscard]] std::uint64_t packed_length_bytes() const noexcept;

    [[nodiscard]] bool operator==(const EvidenceMedia& other) const noexcept = default;

private:
    std::uint32_t width_;
    std::uint32_t height_;
    EvidenceEncoding encoding_;
    EvidenceChannelLayout channel_layout_;
    EvidenceComponentType component_type_;
};

}  // namespace nvision::core
