#pragma once

#include "nvision/acquisition/frame.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace nvision::acquisition {

enum class CameraState : std::uint32_t {
    disconnected = 0,
    connecting = 1,
    connected = 2,
    faulted = 3,
};

enum class AcquisitionStatus : std::uint32_t {
    ok = 0,
    invalid_argument,
    invalid_state,
    not_found,
    end_of_stream,
    invalid_data,
    io_error,
    timeout,
    cancelled,
};

struct AcquisitionError final {
    AcquisitionStatus status{AcquisitionStatus::ok};
    std::string code;
    std::string message;
};

struct OperationResult final {
    AcquisitionError error;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] static OperationResult success();
    [[nodiscard]] static OperationResult failure(
        AcquisitionStatus status,
        std::string code,
        std::string message);
};

struct FrameResult final {
    FrameLease frame;
    AcquisitionError error;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] static FrameResult success(FrameLease frame);
    [[nodiscard]] static FrameResult failure(
        AcquisitionStatus status,
        std::string code,
        std::string message);
};

class CancellationState final {
public:
    void cancel() noexcept;
    [[nodiscard]] bool is_cancelled() const noexcept;

private:
    std::atomic_bool cancelled_{false};
};

struct AcquireRequest final {
    std::chrono::nanoseconds timeout;
    std::string frame_role;
    std::shared_ptr<const CancellationState> cancellation;
};

class FileCamera final {
public:
    using TimestampProvider = std::function<std::int64_t()>;

    FileCamera(
        std::string id,
        std::vector<std::filesystem::path> sources,
        bool loop,
        TimestampProvider timestamp_provider = {});

    FileCamera(const FileCamera&) = delete;
    FileCamera& operator=(const FileCamera&) = delete;
    FileCamera(FileCamera&&) = delete;
    FileCamera& operator=(FileCamera&&) = delete;

    [[nodiscard]] const std::string& id() const noexcept;
    [[nodiscard]] CameraState state() const noexcept;
    [[nodiscard]] OperationResult connect();
    [[nodiscard]] FrameResult acquire(const AcquireRequest& request);
    [[nodiscard]] OperationResult disconnect();

private:
    const std::string id_;
    const std::vector<std::filesystem::path> sources_;
    const bool loop_;
    const TimestampProvider timestamp_provider_;
    mutable std::timed_mutex gate_;
    std::atomic<CameraState> state_{CameraState::disconnected};
    std::size_t next_source_index_{0U};
    std::uint64_t sequence_number_{0U};
};

}  // namespace nvision::acquisition
