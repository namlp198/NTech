#include "nvision/core/inspection_outcome.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nvision::core {

namespace {

bool is_valid(const ExecutionStatus status) noexcept {
    return status == ExecutionStatus::success || status == ExecutionStatus::error ||
           status == ExecutionStatus::timeout || status == ExecutionStatus::cancelled;
}

bool is_valid(const InspectionVerdict verdict) noexcept {
    return verdict == InspectionVerdict::ok || verdict == InspectionVerdict::ng ||
           verdict == InspectionVerdict::indeterminate;
}

}  // namespace

InspectionOutcome::InspectionOutcome(
    const ExecutionStatus execution_status,
    const InspectionVerdict verdict,
    std::string error_code)
    : execution_status_(execution_status),
      verdict_(verdict),
      error_code_(std::move(error_code)) {
    if (!is_valid(execution_status_) || !is_valid(verdict_)) {
        throw std::invalid_argument("execution status and inspection verdict must be valid");
    }
    if (execution_status_ == ExecutionStatus::success) {
        if (!error_code_.empty()) {
            throw std::invalid_argument("successful execution must not carry an error code");
        }
        return;
    }

    if (verdict_ != InspectionVerdict::indeterminate) {
        throw std::invalid_argument("failed execution cannot carry an OK or NG verdict");
    }
    const bool error_code_is_blank = error_code_.empty() ||
                                     std::all_of(
                                         error_code_.begin(),
                                         error_code_.end(),
                                         [](const char character) {
                                             return character == ' ' || character == '\t' ||
                                                    character == '\r' || character == '\n';
                                         });
    if (error_code_is_blank) {
        throw std::invalid_argument("failed execution must carry an error code");
    }
}

ExecutionStatus InspectionOutcome::execution_status() const noexcept {
    return execution_status_;
}

InspectionVerdict InspectionOutcome::verdict() const noexcept {
    return verdict_;
}

const std::string& InspectionOutcome::error_code() const noexcept {
    return error_code_;
}

}  // namespace nvision::core
