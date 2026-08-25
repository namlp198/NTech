#include "nvision/core/execution_result.hpp"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                            \
        if (!(condition)) {         \
            std::abort();           \
        }                           \
    } while (false)

namespace {

using nvision::core::CoordinateSpaceId;
using nvision::core::ErrorCategory;
using nvision::core::EvidenceArtifact;
using nvision::core::EvidenceId;
using nvision::core::EvidenceKind;
using nvision::core::ExecutionError;
using nvision::core::ExecutionRecord;
using nvision::core::ExecutionSessionId;
using nvision::core::ExecutionStatus;
using nvision::core::InspectionDecision;
using nvision::core::InspectionId;
using nvision::core::InspectionOutcome;
using nvision::core::InspectionResult;
using nvision::core::InspectionVerdict;
using nvision::core::IntegrityDigest;
using nvision::core::ResourceIntegrity;
using nvision::core::ResultCode;

template <typename Action>
bool rejects_invalid_argument(Action action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

ResourceIntegrity integrity(const std::uint64_t content_length = 128U) {
    return ResourceIntegrity(
        IntegrityDigest::parse(
            "sha256:3bec1d7190001d1dc1f4ef009013b1abafb1d6a7d725840f15c54920e639e313"),
        content_length);
}

EvidenceArtifact evidence(std::string id = "evidence.source") {
    return EvidenceArtifact(
        EvidenceId(std::move(id)),
        EvidenceKind::source_image,
        integrity(),
        CoordinateSpaceId("nvision.frame.01020304.image-pixel"));
}

ExecutionError camera_timeout_error() {
    return ExecutionError(
        ResultCode("CAMERA_TIMEOUT"),
        ErrorCategory::camera,
        "The camera did not deliver a complete frame before the deadline.",
        true);
}

ExecutionRecord successful_execution() {
    return ExecutionRecord(
        ExecutionSessionId("session.result-success"),
        1'000,
        1'200,
        180U,
        ExecutionStatus::success,
        {});
}

ExecutionRecord timed_out_execution() {
    return ExecutionRecord(
        ExecutionSessionId("session.result-timeout"),
        2'000,
        2'500,
        450U,
        ExecutionStatus::timeout,
        {camera_timeout_error()});
}

InspectionId inspection_id() {
    return InspectionId("inspection.test-result");
}

void successful_results_keep_quality_decisions_separate_from_execution() {
    const InspectionResult result(
        inspection_id(),
        successful_execution(),
        InspectionOutcome(ExecutionStatus::success, InspectionVerdict::ng),
        InspectionDecision(ResultCode("WRONG_TYPE"), InspectionVerdict::ng, 0.87),
        {evidence(), EvidenceArtifact(
                         EvidenceId("evidence.overlay"),
                         EvidenceKind::overlay,
                         integrity(64U),
                         CoordinateSpaceId("nvision.frame.01020304.image-pixel")),
         EvidenceArtifact(
             EvidenceId("evidence.trace"),
             EvidenceKind::trace,
             integrity(32U))});

    NVISION_REQUIRE(result.inspection_id().value() == "inspection.test-result");
    NVISION_REQUIRE(result.execution().status() == ExecutionStatus::success);
    NVISION_REQUIRE(result.execution().errors().empty());
    NVISION_REQUIRE(result.execution().started_at_unix_ns() == 1'000);
    NVISION_REQUIRE(result.execution().completed_at_unix_ns() == 1'200);
    NVISION_REQUIRE(result.execution().monotonic_duration_ns() == 180U);
    NVISION_REQUIRE(result.outcome().verdict() == InspectionVerdict::ng);
    NVISION_REQUIRE(result.decision().has_value());
    NVISION_REQUIRE(result.decision()->code().value() == "WRONG_TYPE");
    NVISION_REQUIRE(result.decision()->score() == std::optional<double>(0.87));
    NVISION_REQUIRE(result.evidence().size() == 3U);
    NVISION_REQUIRE(result.evidence()[0].coordinate_space().has_value());
    NVISION_REQUIRE(result.evidence()[1].coordinate_space().has_value());
    NVISION_REQUIRE(!result.evidence()[2].coordinate_space().has_value());
}

void technical_failures_are_indeterminate_and_carry_matching_errors() {
    const InspectionResult result(
        inspection_id(),
        timed_out_execution(),
        InspectionOutcome(
            ExecutionStatus::timeout,
            InspectionVerdict::indeterminate,
            "CAMERA_TIMEOUT"),
        std::nullopt,
        {EvidenceArtifact(
            EvidenceId("evidence.trace"),
            EvidenceKind::trace,
            integrity(32U))});

    NVISION_REQUIRE(result.execution().status() == ExecutionStatus::timeout);
    NVISION_REQUIRE(result.outcome().verdict() == InspectionVerdict::indeterminate);
    NVISION_REQUIRE(!result.decision().has_value());
    NVISION_REQUIRE(result.execution().errors().size() == 1U);
    NVISION_REQUIRE(result.execution().errors()[0].code().value() == "CAMERA_TIMEOUT");
    NVISION_REQUIRE(result.execution().errors()[0].category() == ErrorCategory::camera);
    NVISION_REQUIRE(result.execution().errors()[0].recoverable());
}

void execution_and_value_contracts_reject_invalid_state() {
    NVISION_REQUIRE(rejects_invalid_argument([] { ResultCode invalid("wrong_type"); }));
    NVISION_REQUIRE(rejects_invalid_argument([] { ResultCode invalid("BAD CODE"); }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ExecutionError invalid(
            ResultCode("SYSTEM_FAILURE"),
            ErrorCategory::system,
            "   ",
            false);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ExecutionError invalid(
            ResultCode("SYSTEM_FAILURE"),
            static_cast<ErrorCategory>(999U),
            "Invalid category.",
            false);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ExecutionRecord invalid(
            ExecutionSessionId("session.invalid-time"),
            200,
            100,
            50U,
            ExecutionStatus::error,
            {camera_timeout_error()});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ExecutionRecord invalid(
            ExecutionSessionId("session.success-with-errors"),
            100,
            200,
            50U,
            ExecutionStatus::success,
            {camera_timeout_error()});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ExecutionRecord invalid(
            ExecutionSessionId("session.failure-without-error"),
            100,
            200,
            50U,
            ExecutionStatus::error,
            {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ExecutionRecord invalid(
            ExecutionSessionId("session.invalid-status"),
            100,
            200,
            50U,
            static_cast<ExecutionStatus>(999U),
            {camera_timeout_error()});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionDecision invalid(
            ResultCode("OK"),
            InspectionVerdict::ok,
            std::numeric_limits<double>::infinity());
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionDecision invalid(
            ResultCode("OK"),
            static_cast<InspectionVerdict>(999U));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        EvidenceArtifact invalid(
            EvidenceId("evidence.invalid"),
            static_cast<EvidenceKind>(999U),
            integrity());
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        EvidenceArtifact invalid(
            EvidenceId("evidence.overlay-without-space"),
            EvidenceKind::overlay,
            integrity());
    }));
}

void inspection_results_reject_cross_aggregate_inconsistency() {
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionResult invalid(
            inspection_id(),
            successful_execution(),
            InspectionOutcome(
                ExecutionStatus::error,
                InspectionVerdict::indeterminate,
                "SYSTEM_FAILURE"),
            std::nullopt,
            {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionResult invalid(
            inspection_id(),
            successful_execution(),
            InspectionOutcome(ExecutionStatus::success, InspectionVerdict::ok),
            std::nullopt,
            {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionResult invalid(
            inspection_id(),
            successful_execution(),
            InspectionOutcome(ExecutionStatus::success, InspectionVerdict::ok),
            InspectionDecision(ResultCode("WRONG_TYPE"), InspectionVerdict::ng),
            {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionResult invalid(
            inspection_id(),
            timed_out_execution(),
            InspectionOutcome(
                ExecutionStatus::timeout,
                InspectionVerdict::indeterminate,
                "CAMERA_TIMEOUT"),
            InspectionDecision(ResultCode("NO_RESULT"), InspectionVerdict::indeterminate),
            {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionResult invalid(
            inspection_id(),
            timed_out_execution(),
            InspectionOutcome(
                ExecutionStatus::timeout,
                InspectionVerdict::indeterminate,
                "DIFFERENT_ERROR"),
            std::nullopt,
            {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionResult invalid(
            inspection_id(),
            successful_execution(),
            InspectionOutcome(ExecutionStatus::success, InspectionVerdict::ok),
            InspectionDecision(ResultCode("OK"), InspectionVerdict::ok),
            {evidence("evidence.duplicate"), evidence("evidence.duplicate")});
    }));
}

}  // namespace

int main() {
    successful_results_keep_quality_decisions_separate_from_execution();
    technical_failures_are_indeterminate_and_carry_matching_errors();
    execution_and_value_contracts_reject_invalid_state();
    inspection_results_reject_cross_aggregate_inconsistency();
    return 0;
}
