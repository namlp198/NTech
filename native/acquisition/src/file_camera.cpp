#include "nvision/acquisition/file_camera.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

namespace nvision::acquisition {

namespace {

using SteadyClock = std::chrono::steady_clock;

struct PortableGraymap final {
    std::uint32_t width;
    std::uint32_t height;
    std::vector<std::uint8_t> pixels;
};

std::int64_t utc_now_unix_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string path_to_utf8(const std::filesystem::path& path) {
    const std::u8string value = path.u8string();
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}

std::vector<std::string_view> tokenize_pgm(const std::string& content) {
    std::vector<std::string_view> tokens;
    std::size_t index = 0U;
    while (index < content.size()) {
        const unsigned char current = static_cast<unsigned char>(content[index]);
        if (current == '#') {
            while (index < content.size() && content[index] != '\r' && content[index] != '\n') {
                ++index;
            }
            continue;
        }

        if (std::isspace(current) != 0) {
            ++index;
            continue;
        }

        const std::size_t start = index;
        while (index < content.size()) {
            const unsigned char character = static_cast<unsigned char>(content[index]);
            if (std::isspace(character) != 0 || character == '#') {
                break;
            }
            ++index;
        }

        tokens.emplace_back(content.data() + start, index - start);
    }

    return tokens;
}

std::uint32_t parse_positive(const std::string_view token, const char* name) {
    std::uint32_t parsed = 0U;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (error != std::errc{} || end != token.data() + token.size() || parsed == 0U) {
        throw std::invalid_argument(std::string("PGM ") + name + " must be a positive integer");
    }

    return parsed;
}

std::uint32_t parse_non_negative(const std::string_view token, const std::size_t index) {
    std::uint32_t parsed = 0U;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (error != std::errc{} || end != token.data() + token.size()) {
        throw std::invalid_argument("PGM pixel " + std::to_string(index) + " is not an integer");
    }

    return parsed;
}

PortableGraymap parse_pgm(const std::string& content) {
    const std::vector<std::string_view> tokens = tokenize_pgm(content);
    if (tokens.size() < 4U || tokens[0] != "P2") {
        throw std::invalid_argument("only ASCII PGM (P2) images are supported");
    }

    const std::uint32_t width = parse_positive(tokens[1], "width");
    const std::uint32_t height = parse_positive(tokens[2], "height");
    const std::uint32_t maximum = parse_positive(tokens[3], "maximum value");
    if (maximum > std::numeric_limits<std::uint8_t>::max()) {
        throw std::invalid_argument("PGM maximum value must not exceed 255");
    }

    if (height > std::numeric_limits<std::size_t>::max() / width) {
        throw std::overflow_error("PGM pixel count overflows addressable memory");
    }

    const std::size_t pixel_count = static_cast<std::size_t>(width) * height;
    if (tokens.size() != pixel_count + 4U) {
        throw std::invalid_argument(
            "PGM declares " + std::to_string(pixel_count) + " pixels but contains " +
            std::to_string(tokens.size() - 4U));
    }

    std::vector<std::uint8_t> pixels(pixel_count);
    for (std::size_t index = 0U; index < pixel_count; ++index) {
        const std::uint32_t value = parse_non_negative(tokens[index + 4U], index);
        if (value > maximum) {
            throw std::invalid_argument(
                "PGM pixel " + std::to_string(index) + " is outside the declared range");
        }

        pixels[index] = static_cast<std::uint8_t>(
            (value * std::numeric_limits<std::uint8_t>::max() + (maximum / 2U)) / maximum);
    }

    return PortableGraymap{width, height, std::move(pixels)};
}

std::array<std::uint8_t, 16> create_frame_id(
    const std::string& camera_id,
    const std::string& source_name,
    const std::uint64_t sequence_number) {
    const std::string identity = camera_id + "|" + source_name + "|" +
                                 std::to_string(sequence_number);
    constexpr std::uint64_t fnv_prime = 1099511628211ULL;
    std::uint64_t first = 14695981039346656037ULL;
    std::uint64_t second = 1099511628211ULL;
    for (const unsigned char character : identity) {
        first = (first ^ character) * fnv_prime;
        second = (second ^ static_cast<unsigned char>(character + 0x9DU)) * fnv_prime;
    }

    std::array<std::uint8_t, 16> id{};
    for (std::size_t index = 0U; index < 8U; ++index) {
        const auto shift = static_cast<unsigned int>(index * 8U);
        id[index] = static_cast<std::uint8_t>((first >> shift) & 0xFFU);
        id[index + 8U] = static_cast<std::uint8_t>((second >> shift) & 0xFFU);
    }

    return id;
}

bool cancelled(const AcquireRequest& request) noexcept {
    return request.cancellation && request.cancellation->is_cancelled();
}

FrameResult cancellation_failure() {
    return FrameResult::failure(
        AcquisitionStatus::cancelled,
        "ACQ-003",
        "frame acquisition was cancelled");
}

FrameResult timeout_failure() {
    return FrameResult::failure(
        AcquisitionStatus::timeout,
        "ACQ-002",
        "frame acquisition exceeded its timeout");
}

}  // namespace

bool OperationResult::succeeded() const noexcept {
    return error.status == AcquisitionStatus::ok;
}

OperationResult OperationResult::success() {
    return {};
}

OperationResult OperationResult::failure(
    const AcquisitionStatus status,
    std::string code,
    std::string message) {
    return OperationResult{AcquisitionError{status, std::move(code), std::move(message)}};
}

bool FrameResult::succeeded() const noexcept {
    return error.status == AcquisitionStatus::ok && static_cast<bool>(frame);
}

FrameResult FrameResult::success(FrameLease frame) {
    if (!frame) {
        throw std::invalid_argument("successful frame result requires a frame");
    }

    return FrameResult{std::move(frame), {}};
}

FrameResult FrameResult::failure(
    const AcquisitionStatus status,
    std::string code,
    std::string message) {
    return FrameResult{{}, AcquisitionError{status, std::move(code), std::move(message)}};
}

void CancellationState::cancel() noexcept {
    cancelled_.store(true, std::memory_order_release);
}

bool CancellationState::is_cancelled() const noexcept {
    return cancelled_.load(std::memory_order_acquire);
}

FileCamera::FileCamera(
    std::string id,
    std::vector<std::filesystem::path> sources,
    const bool loop,
    TimestampProvider timestamp_provider)
    : id_(std::move(id)),
      sources_(std::move(sources)),
      loop_(loop),
      timestamp_provider_(timestamp_provider ? std::move(timestamp_provider) : utc_now_unix_ns) {
    if (id_.empty()) {
        throw std::invalid_argument("File Camera identifier is required");
    }

    if (sources_.empty()) {
        throw std::invalid_argument("File Camera requires at least one source");
    }

    if (std::any_of(sources_.begin(), sources_.end(), [](const auto& source) {
            return source.empty();
        })) {
        throw std::invalid_argument("File Camera source paths are required");
    }
}

const std::string& FileCamera::id() const noexcept {
    return id_;
}

CameraState FileCamera::state() const noexcept {
    return state_.load(std::memory_order_acquire);
}

OperationResult FileCamera::connect() {
    const std::lock_guard<std::timed_mutex> lock(gate_);
    state_.store(CameraState::connecting, std::memory_order_release);
    for (const std::filesystem::path& source : sources_) {
        std::error_code error;
        const bool exists = std::filesystem::is_regular_file(source, error);
        if (error || !exists) {
            state_.store(CameraState::faulted, std::memory_order_release);
            return OperationResult::failure(
                AcquisitionStatus::not_found,
                "FILECAM-002",
                "File Camera source does not exist: '" + path_to_utf8(source) + "'");
        }
    }

    next_source_index_ = 0U;
    sequence_number_ = 0U;
    state_.store(CameraState::connected, std::memory_order_release);
    return OperationResult::success();
}

FrameResult FileCamera::acquire(const AcquireRequest& request) {
    if (request.timeout <= std::chrono::nanoseconds::zero() ||
        request.timeout > std::chrono::hours(24)) {
        return FrameResult::failure(
            AcquisitionStatus::invalid_argument,
            "ACQ-007",
            "frame acquisition timeout must be between zero and 24 hours");
    }

    if (request.frame_role.empty()) {
        return FrameResult::failure(
            AcquisitionStatus::invalid_argument,
            "ACQ-007",
            "frame role is required");
    }

    if (cancelled(request)) {
        return cancellation_failure();
    }

    const SteadyClock::time_point deadline = SteadyClock::now() + request.timeout;
    std::unique_lock<std::timed_mutex> lock(gate_, std::defer_lock);
    constexpr auto cancellation_poll_interval = std::chrono::milliseconds(5);
    while (!lock.try_lock_for(cancellation_poll_interval)) {
        if (cancelled(request)) {
            return cancellation_failure();
        }
        if (SteadyClock::now() >= deadline) {
            return timeout_failure();
        }
    }

    if (cancelled(request)) {
        return cancellation_failure();
    }
    if (SteadyClock::now() >= deadline) {
        return timeout_failure();
    }

    if (state() != CameraState::connected) {
        return FrameResult::failure(
            AcquisitionStatus::invalid_state,
            "FILECAM-001",
            "File Camera '" + id_ + "' must be connected before grabbing a frame");
    }

    if (next_source_index_ >= sources_.size()) {
        if (!loop_) {
            return FrameResult::failure(
                AcquisitionStatus::end_of_stream,
                "FILECAM-003",
                "File Camera '" + id_ + "' reached the end of its source sequence");
        }
        next_source_index_ = 0U;
    }

    const std::filesystem::path source = sources_[next_source_index_++];
    std::ifstream stream(source, std::ios::binary);
    if (!stream) {
        return FrameResult::failure(
            AcquisitionStatus::io_error,
            "FILECAM-006",
            "File Camera '" + id_ + "' could not open source '" + path_to_utf8(source) + "'");
    }

    const std::string content{
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()};
    if (stream.bad()) {
        return FrameResult::failure(
            AcquisitionStatus::io_error,
            "FILECAM-006",
            "File Camera '" + id_ + "' could not read source '" + path_to_utf8(source) + "'");
    }

    PortableGraymap image{};
    try {
        image = parse_pgm(content);
    } catch (const std::invalid_argument& error) {
        return FrameResult::failure(
            AcquisitionStatus::invalid_data,
            "FILECAM-004",
            "File Camera source '" + path_to_utf8(source) + "' is invalid: " + error.what());
    } catch (const std::overflow_error& error) {
        return FrameResult::failure(
            AcquisitionStatus::invalid_data,
            "FILECAM-004",
            "File Camera source '" + path_to_utf8(source) + "' is invalid: " + error.what());
    }

    if (cancelled(request)) {
        return cancellation_failure();
    }
    if (SteadyClock::now() >= deadline) {
        return timeout_failure();
    }

    if (sequence_number_ == std::numeric_limits<std::uint64_t>::max()) {
        return FrameResult::failure(
            AcquisitionStatus::invalid_state,
            "FILECAM-007",
            "File Camera sequence number overflowed");
    }

    const std::uint64_t sequence = ++sequence_number_;
    const std::int64_t timestamp = timestamp_provider_();
    const std::string source_name = path_to_utf8(source.filename());
    auto buffer = std::make_shared<const FrameBuffer>(std::move(image.pixels));
    auto frame = std::make_shared<const Frame>(
        create_frame_id(id_, source_name, sequence),
        id_,
        sequence,
        timestamp,
        timestamp,
        image.width,
        image.height,
        image.width,
        PixelFormat::mono8,
        std::move(buffer),
        FrameStatus::complete,
        TimestampQuality::host_only,
        FrameMetadata{source_name, path_to_utf8(source), request.frame_role});
    return FrameResult::success(std::move(frame));
}

OperationResult FileCamera::disconnect() {
    const std::lock_guard<std::timed_mutex> lock(gate_);
    state_.store(CameraState::disconnected, std::memory_order_release);
    return OperationResult::success();
}

}  // namespace nvision::acquisition
