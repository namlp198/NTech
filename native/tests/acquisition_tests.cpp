#include "nvision/acquisition/file_camera.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                            \
        if (!(condition)) {         \
            std::abort();           \
        }                           \
    } while (false)

namespace {

using namespace std::chrono_literals;
using nvision::acquisition::AcquireRequest;
using nvision::acquisition::AcquisitionStatus;
using nvision::acquisition::CameraState;
using nvision::acquisition::CancellationState;
using nvision::acquisition::FileCamera;
using nvision::acquisition::Frame;
using nvision::acquisition::FrameBuffer;
using nvision::acquisition::FrameLease;
using nvision::acquisition::FrameMetadata;
using nvision::acquisition::FrameStatus;
using nvision::acquisition::PixelFormat;
using nvision::acquisition::TimestampQuality;

std::filesystem::path fixture(const char* name) {
    return std::filesystem::path(NVISION_TEST_FIXTURE_DIR) / name;
}

AcquireRequest request(const std::chrono::nanoseconds timeout = 1s) {
    return AcquireRequest{timeout, "wire-marker", {}};
}

void frame_lease_releases_native_buffer_after_last_owner() {
    auto buffer = std::make_shared<const FrameBuffer>(std::vector<std::uint8_t>{1U, 2U, 3U, 4U});
    std::weak_ptr<const FrameBuffer> observed_buffer = buffer;
    FrameLease first = std::make_shared<const Frame>(
        std::array<std::uint8_t, 16>{1U},
        "test-camera",
        1U,
        0,
        0,
        2U,
        2U,
        2U,
        PixelFormat::mono8,
        std::move(buffer),
        FrameStatus::complete,
        TimestampQuality::host_only,
        FrameMetadata{});
    FrameLease second = first;

    first.reset();
    NVISION_REQUIRE(!observed_buffer.expired());
    NVISION_REQUIRE(second->buffer()->bytes()[0] == 1U);

    second.reset();
    NVISION_REQUIRE(observed_buffer.expired());
}

void complete_frame_rejects_an_undersized_buffer() {
    bool rejected = false;
    try {
        auto buffer = std::make_shared<const FrameBuffer>(std::vector<std::uint8_t>(3U));
        const Frame frame(
            std::array<std::uint8_t, 16>{1U},
            "test-camera",
            1U,
            0,
            0,
            2U,
            2U,
            2U,
            PixelFormat::mono8,
            std::move(buffer),
            FrameStatus::complete,
            TimestampQuality::host_only,
            FrameMetadata{});
        static_cast<void>(frame);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    NVISION_REQUIRE(rejected);
}

void file_camera_replays_and_loops_deterministically() {
    FileCamera camera(
        "wire-marker-file",
        {fixture("type01.pgm"), fixture("type04.pgm")},
        true,
        [] { return std::int64_t{0}; });

    NVISION_REQUIRE(camera.state() == CameraState::disconnected);
    NVISION_REQUIRE(camera.connect().succeeded());
    NVISION_REQUIRE(camera.state() == CameraState::connected);

    auto first = camera.acquire(request());
    auto second = camera.acquire(request());
    auto looped = camera.acquire(request());
    NVISION_REQUIRE(first.succeeded());
    NVISION_REQUIRE(second.succeeded());
    NVISION_REQUIRE(looped.succeeded());
    NVISION_REQUIRE(first.frame->sequence_number() == 1U);
    NVISION_REQUIRE(second.frame->sequence_number() == 2U);
    NVISION_REQUIRE(looped.frame->sequence_number() == 3U);
    NVISION_REQUIRE(first.frame->width() == 8U);
    NVISION_REQUIRE(first.frame->height() == 8U);
    NVISION_REQUIRE(first.frame->stride_bytes() == 8U);
    NVISION_REQUIRE(first.frame->pixel_format() == PixelFormat::mono8);
    NVISION_REQUIRE(first.frame->status() == FrameStatus::complete);
    NVISION_REQUIRE(first.frame->captured_at_unix_ns() == 0);
    NVISION_REQUIRE(first.frame->metadata().source_file == "type01.pgm");
    NVISION_REQUIRE(first.frame->metadata().frame_role == "wire-marker");
    NVISION_REQUIRE(first.frame->buffer()->bytes()[0] != second.frame->buffer()->bytes()[0]);

    const auto original_id = first.frame->id();
    NVISION_REQUIRE(camera.connect().succeeded());
    auto replayed = camera.acquire(request());
    NVISION_REQUIRE(replayed.succeeded());
    NVISION_REQUIRE(replayed.frame->id() == original_id);
    NVISION_REQUIRE(camera.disconnect().succeeded());
    NVISION_REQUIRE(camera.state() == CameraState::disconnected);
}

void lifecycle_errors_timeout_and_cancellation_are_explicit() {
    FileCamera camera("single-file", {fixture("type01.pgm")}, false);
    auto before_connect = camera.acquire(request());
    NVISION_REQUIRE(before_connect.error.status == AcquisitionStatus::invalid_state);
    NVISION_REQUIRE(before_connect.error.code == "FILECAM-001");

    NVISION_REQUIRE(camera.connect().succeeded());
    auto timed_out = camera.acquire(request(1ns));
    NVISION_REQUIRE(timed_out.error.status == AcquisitionStatus::timeout);
    NVISION_REQUIRE(timed_out.error.code == "ACQ-002");

    NVISION_REQUIRE(camera.connect().succeeded());
    auto cancellation = std::make_shared<CancellationState>();
    cancellation->cancel();
    auto cancelled = camera.acquire(AcquireRequest{1s, "wire-marker", cancellation});
    NVISION_REQUIRE(cancelled.error.status == AcquisitionStatus::cancelled);
    NVISION_REQUIRE(cancelled.error.code == "ACQ-003");

    NVISION_REQUIRE(camera.connect().succeeded());
    NVISION_REQUIRE(camera.acquire(request()).succeeded());
    auto exhausted = camera.acquire(request());
    NVISION_REQUIRE(exhausted.error.status == AcquisitionStatus::end_of_stream);
    NVISION_REQUIRE(exhausted.error.code == "FILECAM-003");
}

void missing_source_faults_camera() {
    FileCamera camera("missing", {fixture("does-not-exist.pgm")}, false);
    const auto result = camera.connect();
    NVISION_REQUIRE(!result.succeeded());
    NVISION_REQUIRE(result.error.status == AcquisitionStatus::not_found);
    NVISION_REQUIRE(result.error.code == "FILECAM-002");
    NVISION_REQUIRE(camera.state() == CameraState::faulted);
}

}  // namespace

int main() {
    frame_lease_releases_native_buffer_after_last_owner();
    complete_frame_rejects_an_undersized_buffer();
    file_camera_replays_and_loops_deterministically();
    lifecycle_errors_timeout_and_cancellation_are_explicit();
    missing_source_faults_camera();
    return 0;
}
