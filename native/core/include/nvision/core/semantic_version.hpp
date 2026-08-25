#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace nvision::core {

class SemanticVersion final {
public:
    SemanticVersion(
        std::uint32_t major,
        std::uint32_t minor,
        std::uint32_t patch,
        std::string pre_release = {},
        std::string build_metadata = {});

    [[nodiscard]] std::uint32_t major() const noexcept;
    [[nodiscard]] std::uint32_t minor() const noexcept;
    [[nodiscard]] std::uint32_t patch() const noexcept;
    [[nodiscard]] const std::string& pre_release() const noexcept;
    [[nodiscard]] const std::string& build_metadata() const noexcept;
    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] int compare_precedence(const SemanticVersion& other) const;
    [[nodiscard]] bool operator==(const SemanticVersion& other) const noexcept = default;

private:
    std::uint32_t major_;
    std::uint32_t minor_;
    std::uint32_t patch_;
    std::string pre_release_;
    std::string build_metadata_;
};

}  // namespace nvision::core
