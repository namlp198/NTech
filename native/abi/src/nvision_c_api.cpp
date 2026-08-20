#include "nvision/capi/nvision.h"

#include "nvision/core/engine.hpp"

#include <atomic>
#include <cstring>
#include <exception>
#include <new>
#include <string>
#include <string_view>

struct nv_engine final {
    std::atomic_uint32_t reference_count{1};
    nvision::core::Engine implementation;
};

namespace {

thread_local std::string last_error;

nv_status fail(nv_status status, std::string_view message) {
    last_error.assign(message);
    return status;
}

nv_status copy_utf8(
    std::string_view value,
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    if (required_size == nullptr) {
        return fail(NV_STATUS_INVALID_ARGUMENT, "required_size must not be null");
    }

    const std::size_t size_with_terminator = value.size() + 1U;
    *required_size = size_with_terminator;
    if (buffer == nullptr || buffer_capacity < size_with_terminator) {
        return NV_STATUS_BUFFER_TOO_SMALL;
    }

    std::memcpy(buffer, value.data(), value.size());
    buffer[value.size()] = '\0';
    return NV_STATUS_OK;
}

}  // namespace

extern "C" {

std::uint32_t nv_get_abi_version(void) {
    return NVISION_ABI_VERSION;
}

nv_status nv_engine_create(const nv_engine_config* config, nv_engine** engine) {
    if (engine == nullptr) {
        return fail(NV_STATUS_INVALID_ARGUMENT, "engine output must not be null");
    }

    *engine = nullptr;
    if (config == nullptr || config->struct_size < sizeof(nv_engine_config)) {
        return fail(NV_STATUS_INVALID_ARGUMENT, "engine config is missing or too small");
    }

    if (config->requested_abi_version != NVISION_ABI_VERSION) {
        return fail(NV_STATUS_ABI_MISMATCH, "requested ABI version is unsupported");
    }

    try {
        *engine = new nv_engine{};
        last_error.clear();
        return NV_STATUS_OK;
    } catch (const std::bad_alloc&) {
        return fail(NV_STATUS_INTERNAL_ERROR, "native engine allocation failed");
    } catch (const std::exception& error) {
        return fail(NV_STATUS_INTERNAL_ERROR, error.what());
    } catch (...) {
        return fail(NV_STATUS_INTERNAL_ERROR, "unknown native engine creation failure");
    }
}

nv_status nv_engine_retain(nv_engine* engine) {
    if (engine == nullptr) {
        return fail(NV_STATUS_INVALID_ARGUMENT, "engine handle must not be null");
    }

    std::uint32_t observed = engine->reference_count.load(std::memory_order_acquire);
    while (observed != 0U) {
        if (observed == UINT32_MAX) {
            return fail(NV_STATUS_INTERNAL_ERROR, "engine reference count overflow");
        }

        if (engine->reference_count.compare_exchange_weak(
                observed,
                observed + 1U,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return NV_STATUS_OK;
        }
    }

    return fail(NV_STATUS_INVALID_ARGUMENT, "engine handle is already released");
}

void nv_engine_release(nv_engine* engine) {
    if (engine != nullptr &&
        engine->reference_count.fetch_sub(1U, std::memory_order_acq_rel) == 1U) {
        delete engine;
    }
}

nv_status nv_engine_get_info(const nv_engine* engine, nv_engine_info* info) {
    if (engine == nullptr || info == nullptr || info->struct_size < sizeof(nv_engine_info)) {
        return fail(NV_STATUS_INVALID_ARGUMENT, "engine info request is invalid");
    }

    const nvision::core::EngineInfo native_info = engine->implementation.info();
    info->abi_version = native_info.abi_version;
    info->version_major = native_info.version_major;
    info->version_minor = native_info.version_minor;
    info->version_patch = native_info.version_patch;
    info->feature_flags = native_info.feature_flags;
    return NV_STATUS_OK;
}

nv_status nv_engine_get_capabilities_json(
    const nv_engine* engine,
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    if (engine == nullptr) {
        return fail(NV_STATUS_INVALID_ARGUMENT, "engine handle must not be null");
    }

    return copy_utf8(
        engine->implementation.capabilities_json(),
        buffer,
        buffer_capacity,
        required_size);
}

nv_status nv_get_last_error_message(
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    return copy_utf8(last_error, buffer, buffer_capacity, required_size);
}

}  // extern "C"
