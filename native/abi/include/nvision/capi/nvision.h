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
    NV_STATUS_INTERNAL_ERROR = 100
} nv_status;

typedef struct nv_engine nv_engine;

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

NVISION_API nv_status nv_get_last_error_message(
    char* buffer,
    size_t buffer_capacity,
    size_t* required_size);

#ifdef __cplusplus
}
#endif

#endif
