#include "nvision/core/execution_result.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace nvision::core {

namespace {

constexpr std::size_t maximum_result_code_length = 64U;
constexpr std::size_t maximum_error_detail_length = 4'096U;

bool is_ascii_alphanumeric(const char value) noexcept {
    return (value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9');
}

bool is_valid(const ErrorCategory category) noexcept {
    return category == ErrorCategory::configuration || category == ErrorCategory::device ||
           category == ErrorCategory::camera || category == ErrorCategory::acquisition ||
           category == ErrorCategory::plc || category == ErrorCategory::robot ||
           category == ErrorCategory::ai || category == ErrorCategory::vision ||
           category == ErrorCategory::calibration || category == ErrorCategory::workflow ||
           category == ErrorCategory::storage || category == ErrorCategory::communication ||
           category == ErrorCategory::system || category == ErrorCategory::security;
}

bool is_valid(const ExecutionStatus status) noexcept {
    return status == ExecutionStatus::success || status == ExecutionStatus::error ||
           status == ExecutionStatus::timeout || status == ExecutionStatus::cancelled;
}

bool is_valid(const InspectionVerdict verdict) noexcept {
    return verdict == InspectionVerdict::ok || verdict == InspectionVerdict::ng ||
           verdict == InspectionVerdict::indeterminate;
}

bool is_valid(const EvidenceKind kind) noexcept {
    return kind == EvidenceKind::source_image || kind == EvidenceKind::derived_image ||
           kind == EvidenceKind::mask || kind == EvidenceKind::overlay ||
           kind == EvidenceKind::measurement_data || kind == EvidenceKind::trace;
}

bool requires_coordinate_space(const EvidenceKind kind) noexcept {
    return kind == EvidenceKind::source_image || kind == EvidenceKind::derived_image ||
           kind == EvidenceKind::mask || kind == EvidenceKind::overlay;
}

bool is_blank(const std::string_view value) noexcept {
    return value.empty() || std::all_of(value.begin(), value.end(), [](const char character) {
               return character == ' ' || character == '\t' || character == '\r' ||
                      character == '\n';
           });
}

}  // namespace

ResultCode::ResultCode(std::string value) : value_(std::move(value)) {
    if (value_.empty() || value_.size() > maximum_result_code_length ||
        !is_ascii_alphanumeric(value_.front()) || !is_ascii_alphanumeric(value_.back())) {
        throw std::invalid_argument(
            "result code must be 1-64 characters and start/end with uppercase ASCII or a digit");
    }
    if (!std::all_of(value_.begin(), value_.end(), [](const char character) {
            return is_ascii_alphanumeric(character) || character == '_' || character == '-' ||
                   character == '.';
        })) {
        throw std::invalid_argument(
            "result code must use uppercase ASCII letters, digits, '_', '-', or '.'");
    }
}

const std::string& ResultCode::value() const noexcept {
    return value_;
}

ExecutionError::ExecutionError(
    ResultCode code,
    const ErrorCategory category,
    std::string detail,
    const bool recoverable)
    : code_(std::move(code)),
      category_(category),
      detail_(std::move(detail)),
      recoverable_(recoverable) {
    if (!is_valid(category_)) {
        throw std::invalid_argument("execution error category is unsupported");
    }
    if (is_blank(detail_) || detail_.size() > maximum_error_detail_length) {
        throw std::invalid_argument("execution error detail must contain 1-4096 characters");
    }
}

const ResultCode& ExecutionError::code() const noexcept {
    return code_;
}

ErrorCategory ExecutionError::category() const noexcept {
    return category_;
}

const std::string& ExecutionError::detail() const noexcept {
    return detail_;
}

bool ExecutionError::recoverable() const noexcept {
    return recoverable_;
}

ExecutionRecord::ExecutionRecord(
    ExecutionSessionId session_id,
    const std::int64_t started_at_unix_ns,
    const std::int64_t completed_at_unix_ns,
    const std::uint64_t monotonic_duration_ns,
    const ExecutionStatus status,
    std::vector<ExecutionError> errors)
    : session_id_(std::move(session_id)),
      started_at_unix_ns_(started_at_unix_ns),
      completed_at_unix_ns_(completed_at_unix_ns),
      monotonic_duration_ns_(monotonic_duration_ns),
      status_(status),
      errors_(std::move(errors)) {
    if (!is_valid(status_)) {
        throw std::invalid_argument("execution status is unsupported");
    }
    if (started_at_unix_ns_ < 0 || completed_at_unix_ns_ < started_at_unix_ns_) {
        throw std::invalid_argument("execution timestamps are invalid");
    }
    if (status_ == ExecutionStatus::success && !errors_.empty()) {
        throw std::invalid_argument("successful execution must not contain errors");
    }
    if (status_ != ExecutionStatus::success && errors_.empty()) {
        throw std::invalid_argument("failed execution must contain at least one error");
    }
}

const ExecutionSessionId& ExecutionRecord::session_id() const noexcept {
    return session_id_;
}

std::int64_t ExecutionRecord::started_at_unix_ns() const noexcept {
    return started_at_unix_ns_;
}

std::int64_t ExecutionRecord::completed_at_unix_ns() const noexcept {
    return completed_at_unix_ns_;
}

std::uint64_t ExecutionRecord::monotonic_duration_ns() const noexcept {
    return monotonic_duration_ns_;
}

ExecutionStatus ExecutionRecord::status() const noexcept {
    return status_;
}

const std::vector<ExecutionError>& ExecutionRecord::errors() const noexcept {
    return errors_;
}

InspectionDecision::InspectionDecision(
    ResultCode code,
    const InspectionVerdict verdict,
    std::optional<double> score)
    : code_(std::move(code)), verdict_(verdict), score_(score) {
    if (!is_valid(verdict_)) {
        throw std::invalid_argument("inspection decision verdict is unsupported");
    }
    if (score_.has_value() && !std::isfinite(*score_)) {
        throw std::invalid_argument("inspection decision score must be finite");
    }
}

const ResultCode& InspectionDecision::code() const noexcept {
    return code_;
}

InspectionVerdict InspectionDecision::verdict() const noexcept {
    return verdict_;
}

const std::optional<double>& InspectionDecision::score() const noexcept {
    return score_;
}

EvidenceArtifact::EvidenceArtifact(
    EvidenceId id,
    const EvidenceKind kind,
    ResourceIntegrity integrity,
    std::optional<CoordinateSpaceId> coordinate_space)
    : id_(std::move(id)),
      kind_(kind),
      integrity_(std::move(integrity)),
      coordinate_space_(std::move(coordinate_space)) {
    if (!is_valid(kind_)) {
        throw std::invalid_argument("evidence kind is unsupported");
    }
    if (requires_coordinate_space(kind_) && !coordinate_space_.has_value()) {
        throw std::invalid_argument("spatial evidence requires a coordinate space");
    }
}

const EvidenceId& EvidenceArtifact::id() const noexcept {
    return id_;
}

EvidenceKind EvidenceArtifact::kind() const noexcept {
    return kind_;
}

const ResourceIntegrity& EvidenceArtifact::integrity() const noexcept {
    return integrity_;
}

const std::optional<CoordinateSpaceId>& EvidenceArtifact::coordinate_space() const noexcept {
    return coordinate_space_;
}

InspectionResult::InspectionResult(
    InspectionId inspection_id,
    ExecutionRecord execution,
    InspectionOutcome outcome,
    std::optional<InspectionDecision> decision,
    std::vector<EvidenceArtifact> evidence)
    : inspection_id_(std::move(inspection_id)),
      execution_(std::move(execution)),
      outcome_(std::move(outcome)),
      decision_(std::move(decision)),
      evidence_(std::move(evidence)) {
    if (execution_.status() != outcome_.execution_status()) {
        throw std::invalid_argument("execution record and inspection outcome statuses must match");
    }

    if (execution_.status() == ExecutionStatus::success) {
        if (!decision_.has_value() || decision_->verdict() != outcome_.verdict()) {
            throw std::invalid_argument(
                "successful inspection must contain a decision matching its verdict");
        }
    } else {
        if (decision_.has_value()) {
            throw std::invalid_argument("failed inspection must not contain a quality decision");
        }
        const bool matching_error = std::any_of(
            execution_.errors().begin(),
            execution_.errors().end(),
            [this](const ExecutionError& error) {
                return error.code().value() == outcome_.error_code();
            });
        if (!matching_error) {
            throw std::invalid_argument(
                "inspection outcome error code must match an execution error");
        }
    }

    std::unordered_set<std::string> evidence_ids;
    for (const EvidenceArtifact& artifact : evidence_) {
        if (!evidence_ids.insert(artifact.id().value()).second) {
            throw std::invalid_argument("inspection evidence identifiers must be unique");
        }
    }
}

const InspectionId& InspectionResult::inspection_id() const noexcept {
    return inspection_id_;
}

const ExecutionRecord& InspectionResult::execution() const noexcept {
    return execution_;
}

const InspectionOutcome& InspectionResult::outcome() const noexcept {
    return outcome_;
}

const std::optional<InspectionDecision>& InspectionResult::decision() const noexcept {
    return decision_;
}

const std::vector<EvidenceArtifact>& InspectionResult::evidence() const noexcept {
    return evidence_;
}

}  // namespace nvision::core
