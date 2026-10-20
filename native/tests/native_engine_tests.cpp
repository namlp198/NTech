#include "nvision/capi/nvision.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

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
    NVISION_REQUIRE(info.version_minor == 2U);
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
    NVISION_REQUIRE(capabilities.find("native-acquisition") != std::string::npos);

    NVISION_REQUIRE(nv_engine_retain(engine) == NV_STATUS_OK);
    nv_engine_release(engine);
    nv_engine_release(engine);
}

nv_utf8_view utf8_view(const std::string& value) {
    return nv_utf8_view{value.data(), value.size()};
}

void file_camera_and_frame_ownership_cross_the_c_abi() {
    nv_engine_config engine_config{};
    engine_config.struct_size = sizeof(engine_config);
    engine_config.requested_abi_version = NVISION_ABI_VERSION;
    nv_engine* engine = nullptr;
    NVISION_REQUIRE(nv_engine_create(&engine_config, &engine) == NV_STATUS_OK);

    const std::filesystem::path fixture_root(NVISION_TEST_FIXTURE_DIR);
    const std::vector<std::string> source_storage{
        (fixture_root / "type01.pgm").string(),
        (fixture_root / "type04.pgm").string(),
    };
    const std::vector<nv_utf8_view> sources{
        utf8_view(source_storage[0]),
        utf8_view(source_storage[1]),
    };
    const std::string camera_id = "native-file-camera";
    nv_file_camera_config camera_config{};
    camera_config.struct_size = sizeof(camera_config);
    camera_config.camera_id = utf8_view(camera_id);
    camera_config.sources = sources.data();
    camera_config.source_count = sources.size();
    camera_config.loop = 1U;

    nv_camera* camera = nullptr;
    NVISION_REQUIRE(
        nv_engine_create_file_camera(engine, &camera_config, &camera) == NV_STATUS_OK);
    NVISION_REQUIRE(camera != nullptr);

    nv_engine_release(engine);
    engine = nullptr;

    nv_camera_state state = NV_CAMERA_STATE_FAULTED;
    NVISION_REQUIRE(nv_camera_get_state(camera, &state) == NV_STATUS_OK);
    NVISION_REQUIRE(state == NV_CAMERA_STATE_DISCONNECTED);
    NVISION_REQUIRE(nv_camera_connect(camera) == NV_STATUS_OK);
    NVISION_REQUIRE(nv_camera_get_state(camera, &state) == NV_STATUS_OK);
    NVISION_REQUIRE(state == NV_CAMERA_STATE_CONNECTED);

    const std::string frame_role = "wire-marker";
    nv_acquire_request request{};
    request.struct_size = sizeof(request);
    request.timeout_ns = 1'000'000'000ULL;
    request.frame_role = utf8_view(frame_role);

    nv_frame* frame = nullptr;
    NVISION_REQUIRE(nv_camera_acquire(camera, &request, &frame) == NV_STATUS_OK);
    NVISION_REQUIRE(frame != nullptr);

    nv_frame_info info{};
    info.struct_size = sizeof(info);
    NVISION_REQUIRE(nv_frame_get_info(frame, &info) == NV_STATUS_OK);
    NVISION_REQUIRE(info.sequence_number == 1U);
    NVISION_REQUIRE(info.width == 8U);
    NVISION_REQUIRE(info.height == 8U);
    NVISION_REQUIRE(info.stride_bytes == 8U);
    NVISION_REQUIRE(info.pixel_format == NV_PIXEL_FORMAT_MONO8);
    NVISION_REQUIRE(info.status == NV_FRAME_STATUS_COMPLETE);
    NVISION_REQUIRE(info.buffer_length == 64U);

    const std::uint8_t* pixels = nullptr;
    std::size_t pixel_count = 0U;
    NVISION_REQUIRE(nv_frame_get_buffer(frame, &pixels, &pixel_count) == NV_STATUS_OK);
    NVISION_REQUIRE(pixels != nullptr);
    NVISION_REQUIRE(pixel_count == 64U);
    NVISION_REQUIRE(pixels[0] == 12U);

    std::size_t metadata_size = 0U;
    NVISION_REQUIRE(
        nv_frame_get_metadata_json(frame, nullptr, 0U, &metadata_size) ==
        NV_STATUS_BUFFER_TOO_SMALL);
    std::string metadata(metadata_size, '\0');
    NVISION_REQUIRE(nv_frame_get_metadata_json(
                        frame,
                        metadata.data(),
                        metadata.size(),
                        &metadata_size) == NV_STATUS_OK);
    NVISION_REQUIRE(metadata.find("type01.pgm") != std::string::npos);
    NVISION_REQUIRE(metadata.find("wire-marker") != std::string::npos);

    NVISION_REQUIRE(nv_frame_retain(frame) == NV_STATUS_OK);
    nv_frame_release(frame);
    NVISION_REQUIRE(nv_frame_get_info(frame, &info) == NV_STATUS_OK);
    nv_frame_release(frame);

    nv_cancel_source* cancellation = nullptr;
    NVISION_REQUIRE(nv_cancel_source_create(&cancellation) == NV_STATUS_OK);
    NVISION_REQUIRE(nv_cancel_source_cancel(cancellation) == NV_STATUS_OK);
    request.cancellation = cancellation;
    frame = nullptr;
    NVISION_REQUIRE(nv_camera_acquire(camera, &request, &frame) == NV_STATUS_CANCELLED);
    NVISION_REQUIRE(frame == nullptr);
    nv_cancel_source_release(cancellation);

    NVISION_REQUIRE(nv_camera_disconnect(camera) == NV_STATUS_OK);
    NVISION_REQUIRE(nv_camera_get_state(camera, &state) == NV_STATUS_OK);
    NVISION_REQUIRE(state == NV_CAMERA_STATE_DISCONNECTED);
    nv_camera_release(camera);
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
    file_camera_and_frame_ownership_cross_the_c_abi();
    return 0;
}
