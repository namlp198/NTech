#pragma once

#include "nvision/core/identifier.hpp"
#include "nvision/core/semantic_version.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace nvision::core {

enum class IntegrityAlgorithm : std::uint32_t {
    sha256 = 0,
};

class IntegrityDigest final {
public:
    IntegrityDigest(IntegrityAlgorithm algorithm, std::string hex_digest);

    [[nodiscard]] static IntegrityDigest parse(std::string_view value);
    [[nodiscard]] IntegrityAlgorithm algorithm() const noexcept;
    [[nodiscard]] const std::string& hex_digest() const noexcept;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] bool operator==(const IntegrityDigest& other) const noexcept = default;

private:
    IntegrityAlgorithm algorithm_;
    std::string hex_digest_;
};

class ResourceIntegrity final {
public:
    ResourceIntegrity(IntegrityDigest digest, std::uint64_t content_length_bytes);

    [[nodiscard]] const IntegrityDigest& digest() const noexcept;
    [[nodiscard]] std::uint64_t content_length_bytes() const noexcept;
    [[nodiscard]] bool operator==(const ResourceIntegrity& other) const noexcept = default;

private:
    IntegrityDigest digest_;
    std::uint64_t content_length_bytes_;
};

enum class ResourceKind : std::uint32_t {
    resource = 0,
    model = 1,
    template_asset = 2,
    calibration = 3,
    dictionary = 4,
};

class ResourceReference final {
public:
    ResourceReference(
        ResourceId id,
        SemanticVersion version,
        ResourceKind kind,
        ResourceIntegrity integrity);

    [[nodiscard]] const ResourceId& id() const noexcept;
    [[nodiscard]] const SemanticVersion& version() const noexcept;
    [[nodiscard]] ResourceKind kind() const noexcept;
    [[nodiscard]] const ResourceIntegrity& integrity() const noexcept;
    [[nodiscard]] bool operator==(const ResourceReference& other) const noexcept = default;

private:
    ResourceId id_;
    SemanticVersion version_;
    ResourceKind kind_;
    ResourceIntegrity integrity_;
};

}  // namespace nvision::core
