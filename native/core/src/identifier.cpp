#include "nvision/core/identifier.hpp"

#include <stdexcept>

namespace nvision::core::detail {

namespace {

constexpr std::size_t maximum_identifier_length = 128U;

bool is_ascii_alphanumeric(const char value) noexcept {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9');
}

}  // namespace

void validate_stable_identifier(const std::string_view value) {
    if (value.empty()) {
        throw std::invalid_argument("identifier must not be empty");
    }
    if (value.size() > maximum_identifier_length) {
        throw std::invalid_argument("identifier must not exceed 128 characters");
    }
    if (!is_ascii_alphanumeric(value.front()) || !is_ascii_alphanumeric(value.back())) {
        throw std::invalid_argument("identifier must start and end with an ASCII letter or digit");
    }

    for (const char character : value) {
        const bool separator = character == '.' || character == '-' || character == '_';
        if ((!is_ascii_alphanumeric(character) && !separator) ||
            (character >= 'A' && character <= 'Z')) {
            throw std::invalid_argument(
                "identifier must use lowercase ASCII letters, digits, '.', '-', or '_'");
        }
    }
}

}  // namespace nvision::core::detail
