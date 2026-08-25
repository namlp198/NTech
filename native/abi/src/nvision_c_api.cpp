#include "nvision/capi/nvision.h"

#include "nvision/acquisition/file_camera.hpp"
#include "nvision/core/engine.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct nv_engine final {
    std::atomic_uint32_t reference_count{1};
    nvision::core::Engine implementation;
};

struct nv_camera final {
    nv_camera(nv_engine* engine, std::unique_ptr<nvision::acquisition::FileCamera> camera)
        : owner(engine), implementation(std::move(camera)) {}

    ~nv_camera() {
        nv_engine_release(owner);
    }

    std::atomic_uint32_t reference_count{1};
    nv_engine* owner;
    std::unique_ptr<nvision::acquisition::FileCamera> implementation;
};

struct nv_frame final {
    explicit nv_frame(nvision::acquisition::FrameLease value)
        : implementation(std::move(value)) {}

    std::atomic_uint32_t reference_count{1};
    nvision::acquisition::FrameLease implementation;
};

struct nv_cancel_source final {
    std::atomic_uint32_t reference_count{1};
    std::shared_ptr<nvision::acquisition::CancellationState> implementation{
        std::make_shared<nvision::acquisition::CancellationState>()};
};

namespace {

constexpr std::size_t maximum_error_size = 4096U;
thread_local std::array<char, maximum_error_size> last_error{};
thread_local std::size_t last_error_length = 0U;

void set_last_error(const std::string_view message) noexcept {
    last_error_length = std::min(message.size(), last_error.size() - 1U);
    std::memcpy(last_error.data(), message.data(), last_error_length);
    last_error[last_error_length] = '\0';
}

nv_status fail(const nv_status status, const std::string_view message) noexcept {
    set_last_error(message);
    return status;
}

nv_status succeed() noexcept {
    last_error_length = 0U;
    last_error[0] = '\0';
    return NV_STATUS_OK;
}

nv_status copy_utf8(
    const std::string_view value,
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) noexcept {
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

template <typename Action>
nv_status protect(Action&& action) noexcept {
    try {
        return std::forward<Action>(action)();
    } catch (const std::bad_alloc&) {
        return fail(NV_STATUS_INTERNAL_ERROR, "native allocation failed");
    } catch (const std::exception& error) {
        return fail(NV_STATUS_INTERNAL_ERROR, error.what());
    } catch (...) {
        return fail(NV_STATUS_INTERNAL_ERROR, "unknown native failure");
    }
}

template <typename Handle>
nv_status retain(Handle* handle, const std::string_view null_message) noexcept {
    if (handle == nullptr) {
        return fail(NV_STATUS_INVALID_ARGUMENT, null_message);
    }

    std::uint32_t observed = handle->reference_count.load(std::memory_order_acquire);
    while (observed != 0U) {
        if (observed == std::numeric_limits<std::uint32_t>::max()) {
            return fail(NV_STATUS_INTERNAL_ERROR, "native handle reference count overflow");
        }

        if (handle->reference_count.compare_exchange_weak(
                observed,
                observed + 1U,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return succeed();
        }
    }

    return fail(NV_STATUS_INVALID_ARGUMENT, "native handle is already released");
}

template <typename Handle>
void release(Handle* handle) noexcept {
    if (handle != nullptr &&
        handle->reference_count.fetch_sub(1U, std::memory_order_acq_rel) == 1U) {
        delete handle;
    }
}

std::string from_utf8_view(const nv_utf8_view value, const char* name) {
    if (value.data == nullptr && value.length != 0U) {
        throw std::invalid_argument(std::string(name) + " has null data");
    }

    return value.length == 0U ? std::string{} : std::string(value.data, value.length);
}

std::filesystem::path path_from_utf8(const nv_utf8_view value) {
    const std::string source = from_utf8_view(value, "source path");
    const auto* begin = reinterpret_cast<const char8_t*>(source.data());
    return std::filesystem::path(std::u8string(begin, begin + source.size()));
}

nv_status map_acquisition_error(const nvision::acquisition::AcquisitionError& error) {
    const nv_status status = [&error] {
        using nvision::acquisition::AcquisitionStatus;
        switch (error.status) {
        case AcquisitionStatus::ok:
            return NV_STATUS_OK;
        case AcquisitionStatus::invalid_argument:
            return NV_STATUS_INVALID_ARGUMENT;
        case AcquisitionStatus::invalid_state:
            return NV_STATUS_INVALID_STATE;
        case AcquisitionStatus::not_found:
            return NV_STATUS_NOT_FOUND;
        case AcquisitionStatus::end_of_stream:
            return NV_STATUS_END_OF_STREAM;
        case AcquisitionStatus::invalid_data:
            return NV_STATUS_INVALID_DATA;
        case AcquisitionStatus::io_error:
            return NV_STATUS_IO_ERROR;
        case AcquisitionStatus::timeout:
            return NV_STATUS_TIMEOUT;
        case AcquisitionStatus::cancelled:
            return NV_STATUS_CANCELLED;
        }
        return NV_STATUS_INTERNAL_ERROR;
    }();

    if (status == NV_STATUS_OK) {
        return succeed();
    }

    return fail(status, error.code + ": " + error.message);
}

}  // namespace

extern "C" {

std::uint32_t nv_get_abi_version(void) {
    return NVISION_ABI_VERSION;
}

nv_status nv_engine_create(const nv_engine_config* config, nv_engine** engine) {
    return protect([&] {
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

        *engine = new nv_engine{};
        return succeed();
    });
}

nv_status nv_engine_retain(nv_engine* engine) {
    return retain(engine, "engine handle must not be null");
}

void nv_engine_release(nv_engine* engine) {
    release(engine);
}

nv_status nv_engine_get_info(const nv_engine* engine, nv_engine_info* info) {
    return protect([&] {
        if (engine == nullptr || info == nullptr || info->struct_size < sizeof(nv_engine_info)) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "engine info request is invalid");
        }

        const nvision::core::EngineInfo native_info = engine->implementation.info();
        info->abi_version = native_info.abi_version;
        info->version_major = native_info.version_major;
        info->version_minor = native_info.version_minor;
        info->version_patch = native_info.version_patch;
        info->feature_flags = native_info.feature_flags;
        return succeed();
    });
}

nv_status nv_engine_get_capabilities_json(
    const nv_engine* engine,
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    return protect([&] {
        if (engine == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "engine handle must not be null");
        }

        return copy_utf8(
            engine->implementation.capabilities_json(),
            buffer,
            buffer_capacity,
            required_size);
    });
}

nv_status nv_engine_create_file_camera(
    nv_engine* engine,
    const nv_file_camera_config* config,
    nv_camera** camera) {
    return protect([&] {
        if (camera == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "camera output must not be null");
        }
        *camera = nullptr;

        if (engine == nullptr || config == nullptr ||
            config->struct_size < sizeof(nv_file_camera_config)) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "File Camera request is invalid");
        }
        if (config->sources == nullptr || config->source_count == 0U) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "File Camera requires at least one source");
        }

        std::string camera_id = from_utf8_view(config->camera_id, "camera identifier");
        std::vector<std::filesystem::path> sources;
        sources.reserve(config->source_count);
        for (std::size_t index = 0U; index < config->source_count; ++index) {
            sources.push_back(path_from_utf8(config->sources[index]));
        }

        auto implementation = std::make_unique<nvision::acquisition::FileCamera>(
            std::move(camera_id),
            std::move(sources),
            config->loop != 0U);
        const nv_status retain_status = nv_engine_retain(engine);
        if (retain_status != NV_STATUS_OK) {
            return retain_status;
        }

        try {
            *camera = new nv_camera(engine, std::move(implementation));
        } catch (...) {
            nv_engine_release(engine);
            throw;
        }
        return succeed();
    });
}

nv_status nv_camera_retain(nv_camera* camera) {
    return retain(camera, "camera handle must not be null");
}

void nv_camera_release(nv_camera* camera) {
    release(camera);
}

nv_status nv_camera_get_state(const nv_camera* camera, nv_camera_state* state) {
    return protect([&] {
        if (camera == nullptr || state == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "camera state request is invalid");
        }

        *state = static_cast<nv_camera_state>(camera->implementation->state());
        return succeed();
    });
}

nv_status nv_camera_connect(nv_camera* camera) {
    return protect([&] {
        if (camera == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "camera handle must not be null");
        }

        const nvision::acquisition::OperationResult result = camera->implementation->connect();
        return result.succeeded() ? succeed() : map_acquisition_error(result.error);
    });
}

nv_status nv_camera_acquire(
    nv_camera* camera,
    const nv_acquire_request* request,
    nv_frame** frame) {
    return protect([&] {
        if (frame == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "frame output must not be null");
        }
        *frame = nullptr;

        if (camera == nullptr || request == nullptr ||
            request->struct_size < sizeof(nv_acquire_request) || request->timeout_ns == 0U ||
            request->timeout_ns > static_cast<std::uint64_t>(
                                      std::numeric_limits<std::int64_t>::max())) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "frame acquisition request is invalid");
        }

        std::shared_ptr<const nvision::acquisition::CancellationState> cancellation;
        if (request->cancellation != nullptr) {
            cancellation = request->cancellation->implementation;
        }

        const nvision::acquisition::AcquireRequest native_request{
            std::chrono::nanoseconds(static_cast<std::int64_t>(request->timeout_ns)),
            from_utf8_view(request->frame_role, "frame role"),
            std::move(cancellation),
        };
        nvision::acquisition::FrameResult result = camera->implementation->acquire(native_request);
        if (!result.succeeded()) {
            return map_acquisition_error(result.error);
        }

        *frame = new nv_frame(std::move(result.frame));
        return succeed();
    });
}

nv_status nv_camera_disconnect(nv_camera* camera) {
    return protect([&] {
        if (camera == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "camera handle must not be null");
        }

        const nvision::acquisition::OperationResult result = camera->implementation->disconnect();
        return result.succeeded() ? succeed() : map_acquisition_error(result.error);
    });
}

nv_status nv_cancel_source_create(nv_cancel_source** cancellation) {
    return protect([&] {
        if (cancellation == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "cancellation output must not be null");
        }

        *cancellation = new nv_cancel_source{};
        return succeed();
    });
}

nv_status nv_cancel_source_retain(nv_cancel_source* cancellation) {
    return retain(cancellation, "cancellation handle must not be null");
}

nv_status nv_cancel_source_cancel(nv_cancel_source* cancellation) {
    return protect([&] {
        if (cancellation == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "cancellation handle must not be null");
        }

        cancellation->implementation->cancel();
        return succeed();
    });
}

void nv_cancel_source_release(nv_cancel_source* cancellation) {
    release(cancellation);
}

nv_status nv_frame_retain(nv_frame* frame) {
    return retain(frame, "frame handle must not be null");
}

void nv_frame_release(nv_frame* frame) {
    release(frame);
}

nv_status nv_frame_get_info(const nv_frame* frame, nv_frame_info* info) {
    return protect([&] {
        if (frame == nullptr || info == nullptr || info->struct_size < sizeof(nv_frame_info)) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "frame info request is invalid");
        }

        const nvision::acquisition::Frame& value = *frame->implementation;
        std::copy(value.id().begin(), value.id().end(), info->id);
        info->sequence_number = value.sequence_number();
        info->captured_at_unix_ns = value.captured_at_unix_ns();
        info->received_at_unix_ns = value.received_at_unix_ns();
        info->width = value.width();
        info->height = value.height();
        info->stride_bytes = value.stride_bytes();
        info->pixel_format = static_cast<nv_pixel_format>(value.pixel_format());
        info->status = static_cast<nv_frame_status>(value.status());
        info->timestamp_quality = static_cast<nv_timestamp_quality>(value.timestamp_quality());
        info->buffer_length = value.buffer()->bytes().size();
        return succeed();
    });
}

nv_status nv_frame_get_camera_id(
    const nv_frame* frame,
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    return protect([&] {
        if (frame == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "frame handle must not be null");
        }
        return copy_utf8(
            frame->implementation->camera_id(), buffer, buffer_capacity, required_size);
    });
}

nv_status nv_frame_get_metadata_json(
    const nv_frame* frame,
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    return protect([&] {
        if (frame == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "frame handle must not be null");
        }
        const std::string metadata = frame->implementation->metadata_json();
        return copy_utf8(metadata, buffer, buffer_capacity, required_size);
    });
}

nv_status nv_frame_get_buffer(
    const nv_frame* frame,
    const std::uint8_t** data,
    std::size_t* length) {
    return protect([&] {
        if (frame == nullptr || data == nullptr || length == nullptr) {
            return fail(NV_STATUS_INVALID_ARGUMENT, "frame buffer request is invalid");
        }

        const std::span<const std::uint8_t> bytes = frame->implementation->buffer()->bytes();
        *data = bytes.data();
        *length = bytes.size();
        return succeed();
    });
}

nv_status nv_get_last_error_message(
    char* buffer,
    const std::size_t buffer_capacity,
    std::size_t* required_size) {
    return copy_utf8(
        std::string_view(last_error.data(), last_error_length),
        buffer,
        buffer_capacity,
        required_size);
}

}  // extern "C"
