#ifndef NVISION_CAPI_NVISION_H
#define NVISION_CAPI_NVISION_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(NVISION_ENGINE_BUILD)
#define NVISION_API __declspec(dllexport)
#else
#define NVISION_API __declspec(dllimport)
#endif
#else
#define NVISION_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NVISION_ABI_VERSION 1U

typedef enum nv_status {
    NV_STATUS_OK = 0,
    NV_STATUS_INVALID_ARGUMENT = 1,
    NV_STATUS_ABI_MISMATCH = 2,
    NV_STATUS_BUFFER_TOO_SMALL = 3,
    NV_STATUS_TIMEOUT = 4,
    NV_STATUS_CANCELLED = 5,
    NV_STATUS_INVALID_STATE = 6,
    NV_STATUS_NOT_FOUND = 7,
    NV_STATUS_END_OF_STREAM = 8,
    NV_STATUS_INVALID_DATA = 9,
    NV_STATUS_IO_ERROR = 10,
    NV_STATUS_INTERNAL_ERROR = 100
} nv_status;

typedef struct nv_engine nv_engine;
typedef struct nv_camera nv_camera;
typedef struct nv_frame nv_frame;
typedef struct nv_cancel_source nv_cancel_source;

typedef struct nv_utf8_view {
    const char* data;
    size_t length;
} nv_utf8_view;

typedef enum nv_camera_state {
    NV_CAMERA_STATE_DISCONNECTED = 0,
    NV_CAMERA_STATE_CONNECTING = 1,
    NV_CAMERA_STATE_CONNECTED = 2,
    NV_CAMERA_STATE_FAULTED = 3
} nv_camera_state;

typedef enum nv_pixel_format {
    NV_PIXEL_FORMAT_MONO8 = 0,
    NV_PIXEL_FORMAT_MONO16 = 1,
    NV_PIXEL_FORMAT_RGB8 = 2,
    NV_PIXEL_FORMAT_BGR8 = 3
} nv_pixel_format;

typedef enum nv_frame_status {
    NV_FRAME_STATUS_COMPLETE = 0,
    NV_FRAME_STATUS_INCOMPLETE = 1,
    NV_FRAME_STATUS_CORRUPT = 2,
    NV_FRAME_STATUS_DROPPED = 3,
    NV_FRAME_STATUS_TIMEOUT = 4,
    NV_FRAME_STATUS_CANCELLED = 5
} nv_frame_status;

typedef enum nv_timestamp_quality {
    NV_TIMESTAMP_QUALITY_HOST_ONLY = 0,
    NV_TIMESTAMP_QUALITY_CAMERA_UNSYNCHRONIZED = 1,
    NV_TIMESTAMP_QUALITY_CAMERA_SYNCHRONIZED = 2,
    NV_TIMESTAMP_QUALITY_PTP_SYNCHRONIZED = 3,
    NV_TIMESTAMP_QUALITY_EXTERNAL_CLOCK = 4
} nv_timestamp_quality;

typedef struct nv_engine_config {
    uint32_t struct_size;
    uint32_t requested_abi_version;
    uint64_t reserved[4];
} nv_engine_config;

typedef struct nv_engine_info {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint64_t feature_flags;
} nv_engine_info;

typedef struct nv_file_camera_config {
    uint32_t struct_size;
    nv_utf8_view camera_id;
    const nv_utf8_view* sources;
    size_t source_count;
    uint32_t loop;
    uint32_t reserved0;
    uint64_t reserved[4];
} nv_file_camera_config;

typedef struct nv_acquire_request {
    uint32_t struct_size;
    uint32_t reserved0;
    uint64_t timeout_ns;
    nv_utf8_view frame_role;
    const nv_cancel_source* cancellation;
    uint64_t reserved[4];
} nv_acquire_request;

typedef struct nv_frame_info {
    uint32_t struct_size;
    uint8_t id[16];
    uint64_t sequence_number;
    int64_t captured_at_unix_ns;
    int64_t received_at_unix_ns;
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    nv_pixel_format pixel_format;
    nv_frame_status status;
    nv_timestamp_quality timestamp_quality;
    uint64_t buffer_length;
} nv_frame_info;

NVISION_API uint32_t nv_get_abi_version(void);

NVISION_API nv_status nv_engine_create(
    const nv_engine_config* config,
    nv_engine** engine);

NVISION_API nv_status nv_engine_retain(nv_engine* engine);

NVISION_API void nv_engine_release(nv_engine* engine);

NVISION_API nv_status nv_engine_get_info(
    const nv_engine* engine,
    nv_engine_info* info);

NVISION_API nv_status nv_engine_get_capabilities_json(
    const nv_engine* engine,
    char* buffer,
    size_t buffer_capacity,
    size_t* required_size);

NVISION_API nv_status nv_engine_create_file_camera(
    nv_engine* engine,
    const nv_file_camera_config* config,
    nv_camera** camera);

NVISION_API nv_status nv_camera_retain(nv_camera* camera);

NVISION_API void nv_camera_release(nv_camera* camera);

NVISION_API nv_status nv_camera_get_state(
    const nv_camera* camera,
    nv_camera_state* state);

NVISION_API nv_status nv_camera_connect(nv_camera* camera);

NVISION_API nv_status nv_camera_acquire(
    nv_camera* camera,
    const nv_acquire_request* request,
    nv_frame** frame);

NVISION_API nv_status nv_camera_disconnect(nv_camera* camera);

NVISION_API nv_status nv_cancel_source_create(nv_cancel_source** cancellation);

NVISION_API nv_status nv_cancel_source_retain(nv_cancel_source* cancellation);

NVISION_API nv_status nv_cancel_source_cancel(nv_cancel_source* cancellation);

NVISION_API void nv_cancel_source_release(nv_cancel_source* cancellation);

NVISION_API nv_status nv_frame_retain(nv_frame* frame);

NVISION_API void nv_frame_release(nv_frame* frame);

NVISION_API nv_status nv_frame_get_info(
    const nv_frame* frame,
    nv_frame_info* info);

NVISION_API nv_status nv_frame_get_camera_id(
    const nv_frame* frame,
    char* buffer,
    size_t buffer_capacity,
    size_t* required_size);

NVISION_API nv_status nv_frame_get_metadata_json(
    const nv_frame* frame,
    char* buffer,
    size_t buffer_capacity,
    size_t* required_size);

NVISION_API nv_status nv_frame_get_buffer(
    const nv_frame* frame,
    const uint8_t** data,
    size_t* length);

NVISION_API nv_status nv_get_last_error_message(
    char* buffer,
    size_t buffer_capacity,
    size_t* required_size);

#ifdef __cplusplus
}
#endif

#endif
