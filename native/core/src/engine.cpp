#include "nvision/core/engine.hpp"

namespace nvision::core {

namespace {

constexpr std::uint32_t abi_version = 1;
constexpr std::uint64_t feature_native_core = 1ULL << 0U;
constexpr std::string_view capabilities =
    R"({"abiVersion":1,"engineVersion":"0.1.0","features":["native-core","c-abi"]})";

}  // namespace

EngineInfo Engine::info() const noexcept {
    return EngineInfo{
        .abi_version = abi_version,
        .version_major = 0,
        .version_minor = 1,
        .version_patch = 0,
        .feature_flags = feature_native_core,
    };
}

std::string_view Engine::capabilities_json() const noexcept {
    return capabilities;
}

}  // namespace nvision::core
