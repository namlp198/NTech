#include "nvision/core/semantic_version.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nvision::core {

namespace {

bool is_ascii_alphanumeric_or_hyphen(const char value) noexcept {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '-';
}

bool is_numeric(const std::string_view value) noexcept {
    return !value.empty() &&
           std::all_of(value.begin(), value.end(), [](const char character) {
               return character >= '0' && character <= '9';
           });
}

void validate_identifiers(const std::string_view value, const bool reject_numeric_leading_zero) {
    if (value.empty()) {
        return;
    }

    std::size_t start = 0U;
    while (start <= value.size()) {
        const std::size_t end = value.find('.', start);
        const std::string_view identifier = value.substr(
            start,
            end == std::string_view::npos ? value.size() - start : end - start);
        if (identifier.empty() ||
            !std::all_of(
                identifier.begin(), identifier.end(), is_ascii_alphanumeric_or_hyphen)) {
            throw std::invalid_argument("semantic version contains an invalid identifier");
        }
        if (reject_numeric_leading_zero && identifier.size() > 1U &&
            identifier.front() == '0' && is_numeric(identifier)) {
            throw std::invalid_argument(
                "numeric pre-release identifiers must not contain leading zeroes");
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }
}

std::vector<std::string_view> split_identifiers(const std::string_view value) {
    std::vector<std::string_view> identifiers;
    std::size_t start = 0U;
    while (start <= value.size()) {
        const std::size_t end = value.find('.', start);
        identifiers.push_back(value.substr(
            start,
            end == std::string_view::npos ? value.size() - start : end - start));
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }
    return identifiers;
}

int compare_numeric_identifier(
    const std::string_view left,
    const std::string_view right) noexcept {
    if (left.size() != right.size()) {
        return left.size() < right.size() ? -1 : 1;
    }
    const int comparison = left.compare(right);
    return comparison < 0 ? -1 : comparison > 0 ? 1 : 0;
}

int compare_pre_release(
    const std::string_view left,
    const std::string_view right) {
    if (left.empty()) {
        return right.empty() ? 0 : 1;
    }
    if (right.empty()) {
        return -1;
    }

    const auto left_identifiers = split_identifiers(left);
    const auto right_identifiers = split_identifiers(right);
    const std::size_t count = std::min(left_identifiers.size(), right_identifiers.size());
    for (std::size_t index = 0U; index < count; ++index) {
        const auto left_identifier = left_identifiers[index];
        const auto right_identifier = right_identifiers[index];
        const bool left_numeric = is_numeric(left_identifier);
        const bool right_numeric = is_numeric(right_identifier);
        int comparison = 0;
        if (left_numeric && right_numeric) {
            comparison = compare_numeric_identifier(left_identifier, right_identifier);
        } else if (left_numeric != right_numeric) {
            comparison = left_numeric ? -1 : 1;
        } else {
            const int lexical = left_identifier.compare(right_identifier);
            comparison = lexical < 0 ? -1 : lexical > 0 ? 1 : 0;
        }
        if (comparison != 0) {
            return comparison;
        }
    }

    if (left_identifiers.size() == right_identifiers.size()) {
        return 0;
    }
    return left_identifiers.size() < right_identifiers.size() ? -1 : 1;
}

template <typename Value>
int compare_value(const Value left, const Value right) noexcept {
    return left < right ? -1 : left > right ? 1 : 0;
}

}  // namespace

SemanticVersion::SemanticVersion(
    const std::uint32_t major,
    const std::uint32_t minor,
    const std::uint32_t patch,
    std::string pre_release,
    std::string build_metadata)
    : major_(major),
      minor_(minor),
      patch_(patch),
      pre_release_(std::move(pre_release)),
      build_metadata_(std::move(build_metadata)) {
    validate_identifiers(pre_release_, true);
    validate_identifiers(build_metadata_, false);
}

std::uint32_t SemanticVersion::major() const noexcept {
    return major_;
}

std::uint32_t SemanticVersion::minor() const noexcept {
    return minor_;
}

std::uint32_t SemanticVersion::patch() const noexcept {
    return patch_;
}

const std::string& SemanticVersion::pre_release() const noexcept {
    return pre_release_;
}

const std::string& SemanticVersion::build_metadata() const noexcept {
    return build_metadata_;
}

std::string SemanticVersion::to_string() const {
    std::string result = std::to_string(major_) + "." + std::to_string(minor_) + "." +
                         std::to_string(patch_);
    if (!pre_release_.empty()) {
        result += "-" + pre_release_;
    }
    if (!build_metadata_.empty()) {
        result += "+" + build_metadata_;
    }
    return result;
}

int SemanticVersion::compare_precedence(const SemanticVersion& other) const {
    if (const int comparison = compare_value(major_, other.major_); comparison != 0) {
        return comparison;
    }
    if (const int comparison = compare_value(minor_, other.minor_); comparison != 0) {
        return comparison;
    }
    if (const int comparison = compare_value(patch_, other.patch_); comparison != 0) {
        return comparison;
    }
    return compare_pre_release(pre_release_, other.pre_release_);
}

}  // namespace nvision::core
