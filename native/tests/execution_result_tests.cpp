#include "nvision/core/evidence_media.hpp"
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
using nvision::core::EvidenceChannelLayout;
using nvision::core::EvidenceComponentType;
using nvision::core::EvidenceEncoding;
using nvision::core::EvidenceId;
using nvision::core::EvidenceKind;
using nvision::core::EvidenceMedia;
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

// 16 by 8 packed mono8 pixels are exactly 128 bytes, which is what the artifact claims to
// have stored. The descriptor and the length have to agree or neither can be trusted.
EvidenceMedia mono_media() {
    return EvidenceMedia(
        16U, 8U, EvidenceEncoding::raw_packed, EvidenceChannelLayout::mono,
        EvidenceComponentType::uint8);
}

EvidenceArtifact evidence(std::string id = "evidence.source") {
    return EvidenceArtifact(
        EvidenceId(std::move(id)),
        EvidenceKind::source_image,
        integrity(),
        "evidence/source.raw",
        mono_media(),
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
        {evidence(),
         EvidenceArtifact(
             EvidenceId("evidence.overlay"),
             EvidenceKind::overlay,
             integrity(48U),
             "evidence/overlay.raw",
             EvidenceMedia(
                 4U, 4U, EvidenceEncoding::raw_packed, EvidenceChannelLayout::bgr,
                 EvidenceComponentType::uint8),
             CoordinateSpaceId("nvision.frame.01020304.image-pixel")),
         EvidenceArtifact(
             EvidenceId("evidence.trace"),
             EvidenceKind::trace,
             integrity(32U),
             "evidence/trace.json")});

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
            integrity(32U),
            "evidence/trace.json")});

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
            integrity(),
            "evidence/invalid.raw");
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        EvidenceArtifact invalid(
            EvidenceId("evidence.overlay-without-space"),
            EvidenceKind::overlay,
            integrity(),
            "evidence/overlay.raw",
            mono_media());
    }));
}

// An artifact nobody can read is not evidence. These are the four ways the descriptor and
// the bytes can disagree, and each one leaves a consumer unable to reconstruct the image.
void evidence_describes_its_own_media_or_is_rejected() {
    const auto space = CoordinateSpaceId("nvision.frame.01020304.image-pixel");

    NVISION_REQUIRE(rejects_invalid_argument([&space] {
        EvidenceArtifact without_media(
            EvidenceId("evidence.image"),
            EvidenceKind::derived_image,
            integrity(),
            "evidence/image.raw",
            std::nullopt,
            space);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        // A trace is not a raster, so a media descriptor on one would describe nothing.
        EvidenceArtifact trace_with_media(
            EvidenceId("evidence.trace"),
            EvidenceKind::trace,
            integrity(),
            "evidence/trace.json",
            mono_media());
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&space] {
        // Packed pixels have exactly one possible length, so 16 by 8 mono8 is never 64.
        EvidenceArtifact wrong_length(
            EvidenceId("evidence.image"),
            EvidenceKind::derived_image,
            integrity(64U),
            "evidence/image.raw",
            mono_media(),
            space);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&space] {
        EvidenceArtifact nowhere(
            EvidenceId("evidence.image"),
            EvidenceKind::derived_image,
            integrity(),
            "   ",
            mono_media(),
            space);
    }));

    const EvidenceArtifact readable = evidence();
    NVISION_REQUIRE(readable.storage_reference() == "evidence/source.raw");
    NVISION_REQUIRE(readable.media().has_value());
    NVISION_REQUIRE(readable.media()->width() == 16U);
    NVISION_REQUIRE(readable.media()->height() == 8U);
    NVISION_REQUIRE(readable.media()->channel_count() == 1U);
    NVISION_REQUIRE(readable.media()->packed_length_bytes() == 128U);
    // Packed pixels are not a picture a reader can open unaided, and say so.
    NVISION_REQUIRE(readable.media()->media_type() == "application/octet-stream");

    const EvidenceMedia encoded(
        640U, 480U, EvidenceEncoding::png, EvidenceChannelLayout::bgr,
        EvidenceComponentType::uint8);
    NVISION_REQUIRE(encoded.media_type() == "image/png");
    // A container compresses, so its length is not predictable from its dimensions and the
    // artifact does not pretend otherwise.
    NVISION_REQUIRE(encoded.packed_length_bytes() == 0U);
    const EvidenceArtifact compressed(
        EvidenceId("evidence.png"),
        EvidenceKind::derived_image,
        integrity(37U),
        "evidence/image.png",
        encoded,
        space);
    NVISION_REQUIRE(compressed.media()->encoding() == EvidenceEncoding::png);

    NVISION_REQUIRE(rejects_invalid_argument([] {
        EvidenceMedia empty(
            0U, 8U, EvidenceEncoding::raw_packed, EvidenceChannelLayout::mono,
            EvidenceComponentType::uint8);
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
    evidence_describes_its_own_media_or_is_rejected();
    inspection_results_reject_cross_aggregate_inconsistency();
    return 0;
}
