#include "nvision/capi/nvision.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#define NVISION_REQUIRE(condition) \
    do {                            \
        if (!(condition)) {         \
            std::abort();           \
        }                           \
    } while (false)

namespace {

void engine_lifecycle_and_capabilities_are_versioned() {
    NVISION_REQUIRE(nv_get_abi_version() == NVISION_ABI_VERSION);

    nv_engine_config config{};
    config.struct_size = sizeof(config);
    config.requested_abi_version = NVISION_ABI_VERSION;

    nv_engine* engine = nullptr;
    NVISION_REQUIRE(nv_engine_create(&config, &engine) == NV_STATUS_OK);
    NVISION_REQUIRE(engine != nullptr);

    nv_engine_info info{};
    info.struct_size = sizeof(info);
    NVISION_REQUIRE(nv_engine_get_info(engine, &info) == NV_STATUS_OK);
    NVISION_REQUIRE(info.abi_version == NVISION_ABI_VERSION);
    NVISION_REQUIRE(info.version_major == 0U);
    NVISION_REQUIRE(info.version_minor == 1U);
    NVISION_REQUIRE(info.feature_flags != 0U);

    std::size_t required_size = 0U;
    NVISION_REQUIRE(nv_engine_get_capabilities_json(engine, nullptr, 0U, &required_size) ==
                    NV_STATUS_BUFFER_TOO_SMALL);
    NVISION_REQUIRE(required_size > 1U);

    std::string capabilities(required_size, '\0');
    NVISION_REQUIRE(nv_engine_get_capabilities_json(
                        engine,
                        capabilities.data(),
                        capabilities.size(),
                        &required_size) == NV_STATUS_OK);
    NVISION_REQUIRE(capabilities.find("native-core") != std::string::npos);

    NVISION_REQUIRE(nv_engine_retain(engine) == NV_STATUS_OK);
    nv_engine_release(engine);
    nv_engine_release(engine);
}

void incompatible_abi_is_rejected_with_diagnostics() {
    nv_engine_config config{};
    config.struct_size = sizeof(config);
    config.requested_abi_version = NVISION_ABI_VERSION + 1U;

    nv_engine* engine = nullptr;
    NVISION_REQUIRE(nv_engine_create(&config, &engine) == NV_STATUS_ABI_MISMATCH);
    NVISION_REQUIRE(engine == nullptr);

    std::size_t required_size = 0U;
    NVISION_REQUIRE(nv_get_last_error_message(nullptr, 0U, &required_size) ==
                    NV_STATUS_BUFFER_TOO_SMALL);

    std::string message(required_size, '\0');
    NVISION_REQUIRE(nv_get_last_error_message(
                        message.data(),
                        message.size(),
                        &required_size) == NV_STATUS_OK);
    NVISION_REQUIRE(message.find("ABI") != std::string::npos);
}

}  // namespace

int main() {
    engine_lifecycle_and_capabilities_are_versioned();
    incompatible_abi_is_rejected_with_diagnostics();
    return 0;
}
