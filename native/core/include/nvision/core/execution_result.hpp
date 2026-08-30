#pragma once

#include "nvision/core/evidence_media.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/inspection_outcome.hpp"
#include "nvision/core/resource_reference.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace nvision::core {

class ResultCode final {
public:
    explicit ResultCode(std::string value);

    [[nodiscard]] const std::string& value() const noexcept;
    [[nodiscard]] bool operator==(const ResultCode& other) const noexcept = default;

private:
    std::string value_;
};

enum class ErrorCategory : std::uint32_t {
    configuration = 0,
    device = 1,
    camera = 2,
    acquisition = 3,
    plc = 4,
    robot = 5,
    ai = 6,
    vision = 7,
    calibration = 8,
    workflow = 9,
    storage = 10,
    communication = 11,
    system = 12,
    security = 13,
};

class ExecutionError final {
public:
    ExecutionError(
        ResultCode code,
        ErrorCategory category,
        std::string detail,
        bool recoverable);

    [[nodiscard]] const ResultCode& code() const noexcept;
    [[nodiscard]] ErrorCategory category() const noexcept;
    [[nodiscard]] const std::string& detail() const noexcept;
    [[nodiscard]] bool recoverable() const noexcept;

private:
    ResultCode code_;
    ErrorCategory category_;
    std::string detail_;
    bool recoverable_;
};

class ExecutionRecord final {
public:
    ExecutionRecord(
        ExecutionSessionId session_id,
        std::int64_t started_at_unix_ns,
        std::int64_t completed_at_unix_ns,
        std::uint64_t monotonic_duration_ns,
        ExecutionStatus status,
        std::vector<ExecutionError> errors);

    [[nodiscard]] const ExecutionSessionId& session_id() const noexcept;
    [[nodiscard]] std::int64_t started_at_unix_ns() const noexcept;
    [[nodiscard]] std::int64_t completed_at_unix_ns() const noexcept;
    [[nodiscard]] std::uint64_t monotonic_duration_ns() const noexcept;
    [[nodiscard]] ExecutionStatus status() const noexcept;
    [[nodiscard]] const std::vector<ExecutionError>& errors() const noexcept;

private:
    ExecutionSessionId session_id_;
    std::int64_t started_at_unix_ns_;
    std::int64_t completed_at_unix_ns_;
    std::uint64_t monotonic_duration_ns_;
    ExecutionStatus status_;
    std::vector<ExecutionError> errors_;
};

class InspectionDecision final {
public:
    InspectionDecision(
        ResultCode code,
        InspectionVerdict verdict,
        std::optional<double> score = std::nullopt);

    [[nodiscard]] const ResultCode& code() const noexcept;
    [[nodiscard]] InspectionVerdict verdict() const noexcept;
    [[nodiscard]] const std::optional<double>& score() const noexcept;

private:
    ResultCode code_;
    InspectionVerdict verdict_;
    std::optional<double> score_;
};

enum class EvidenceKind : std::uint32_t {
    source_image = 0,
    derived_image = 1,
    mask = 2,
    overlay = 3,
    measurement_data = 4,
    trace = 5,
};

// One stored artifact, described well enough that a consumer can find it and read it.
// Image evidence must carry its media descriptor: bytes with no dimensions, layout or
// encoding are not evidence of anything, because nobody downstream can reconstruct what
// was seen. Non-image evidence must not carry one, so a descriptor always means an image.
class EvidenceArtifact final {
public:
    EvidenceArtifact(
        EvidenceId id,
        EvidenceKind kind,
        ResourceIntegrity integrity,
        std::string storage_reference,
        std::optional<EvidenceMedia> media = std::nullopt,
        std::optional<CoordinateSpaceId> coordinate_space = std::nullopt);

    [[nodiscard]] const EvidenceId& id() const noexcept;
    [[nodiscard]] EvidenceKind kind() const noexcept;
    [[nodiscard]] const ResourceIntegrity& integrity() const noexcept;
    [[nodiscard]] const std::string& storage_reference() const noexcept;
    [[nodiscard]] const std::optional<EvidenceMedia>& media() const noexcept;
    [[nodiscard]] const std::optional<CoordinateSpaceId>& coordinate_space() const noexcept;

private:
    EvidenceId id_;
    EvidenceKind kind_;
    ResourceIntegrity integrity_;
    std::string storage_reference_;
    std::optional<EvidenceMedia> media_;
    std::optional<CoordinateSpaceId> coordinate_space_;
};

class InspectionResult final {
public:
    InspectionResult(
        InspectionId inspection_id,
        ExecutionRecord execution,
        InspectionOutcome outcome,
        std::optional<InspectionDecision> decision,
        std::vector<EvidenceArtifact> evidence);

    [[nodiscard]] const InspectionId& inspection_id() const noexcept;
    [[nodiscard]] const ExecutionRecord& execution() const noexcept;
    [[nodiscard]] const InspectionOutcome& outcome() const noexcept;
    [[nodiscard]] const std::optional<InspectionDecision>& decision() const noexcept;
    [[nodiscard]] const std::vector<EvidenceArtifact>& evidence() const noexcept;

private:
    InspectionId inspection_id_;
    ExecutionRecord execution_;
    InspectionOutcome outcome_;
    std::optional<InspectionDecision> decision_;
    std::vector<EvidenceArtifact> evidence_;
};

}  // namespace nvision::core
