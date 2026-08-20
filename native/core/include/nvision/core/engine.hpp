#pragma once

#include <cstdint>
#include <string_view>

namespace nvision::core {

struct EngineInfo final {
    std::uint32_t abi_version;
    std::uint32_t version_major;
    std::uint32_t version_minor;
    std::uint32_t version_patch;
    std::uint64_t feature_flags;
};

class Engine final {
public:
    [[nodiscard]] EngineInfo info() const noexcept;
    [[nodiscard]] std::string_view capabilities_json() const noexcept;
};

}  // namespace nvision::core
