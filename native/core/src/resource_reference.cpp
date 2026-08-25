#include "nvision/core/resource_reference.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nvision::core {

namespace {

constexpr std::string_view sha256_prefix = "sha256:";
constexpr std::size_t sha256_hex_length = 64U;

bool is_valid(const IntegrityAlgorithm algorithm) noexcept {
    return algorithm == IntegrityAlgorithm::sha256;
}

bool is_valid(const ResourceKind kind) noexcept {
    return kind == ResourceKind::resource || kind == ResourceKind::model ||
           kind == ResourceKind::template_asset || kind == ResourceKind::calibration ||
           kind == ResourceKind::dictionary;
}

bool is_lowercase_hex(const char value) noexcept {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f');
}

void validate_digest(const IntegrityAlgorithm algorithm, const std::string_view hex_digest) {
    if (!is_valid(algorithm)) {
        throw std::invalid_argument("integrity algorithm is unsupported");
    }
    if (hex_digest.size() != sha256_hex_length ||
        !std::all_of(hex_digest.begin(), hex_digest.end(), is_lowercase_hex)) {
        throw std::invalid_argument(
            "SHA-256 digest must contain exactly 64 lowercase hexadecimal characters");
    }
}

}  // namespace

IntegrityDigest::IntegrityDigest(
    const IntegrityAlgorithm algorithm,
    std::string hex_digest)
    : algorithm_(algorithm), hex_digest_(std::move(hex_digest)) {
    validate_digest(algorithm_, hex_digest_);
}

IntegrityDigest IntegrityDigest::parse(const std::string_view value) {
    if (!value.starts_with(sha256_prefix)) {
        throw std::invalid_argument("integrity digest must use the canonical 'sha256:' prefix");
    }
    return IntegrityDigest(
        IntegrityAlgorithm::sha256,
        std::string(value.substr(sha256_prefix.size())));
}

IntegrityAlgorithm IntegrityDigest::algorithm() const noexcept {
    return algorithm_;
}

const std::string& IntegrityDigest::hex_digest() const noexcept {
    return hex_digest_;
}

std::string IntegrityDigest::to_string() const {
    return std::string(sha256_prefix) + hex_digest_;
}

ResourceIntegrity::ResourceIntegrity(
    IntegrityDigest digest,
    const std::uint64_t content_length_bytes)
    : digest_(std::move(digest)), content_length_bytes_(content_length_bytes) {
    if (content_length_bytes_ == 0U) {
        throw std::invalid_argument("resource content length must be positive");
    }
}

const IntegrityDigest& ResourceIntegrity::digest() const noexcept {
    return digest_;
}

std::uint64_t ResourceIntegrity::content_length_bytes() const noexcept {
    return content_length_bytes_;
}

ResourceReference::ResourceReference(
    ResourceId id,
    SemanticVersion version,
    const ResourceKind kind,
    ResourceIntegrity integrity)
    : id_(std::move(id)),
      version_(std::move(version)),
      kind_(kind),
      integrity_(std::move(integrity)) {
    if (!is_valid(kind_)) {
        throw std::invalid_argument("resource kind is unsupported");
    }
}

const ResourceId& ResourceReference::id() const noexcept {
    return id_;
}

const SemanticVersion& ResourceReference::version() const noexcept {
    return version_;
}

ResourceKind ResourceReference::kind() const noexcept {
    return kind_;
}

const ResourceIntegrity& ResourceReference::integrity() const noexcept {
    return integrity_;
}

}  // namespace nvision::core
