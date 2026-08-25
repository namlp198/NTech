#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/data_type.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/core/inspection_outcome.hpp"
#include "nvision/core/resource_reference.hpp"
#include "nvision/core/semantic_version.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                            \
        if (!(condition)) {         \
            std::abort();           \
        }                           \
    } while (false)

namespace {

using nvision::core::AxisAlignedRectangle;
using nvision::core::Circle;
using nvision::core::CoordinateSpaceId;
using nvision::core::ExecutionSessionId;
using nvision::core::ExecutionStatus;
using nvision::core::InspectionOutcome;
using nvision::core::InspectionVerdict;
using nvision::core::IntegrityAlgorithm;
using nvision::core::IntegrityDigest;
using nvision::core::Line2D;
using nvision::core::Point2D;
using nvision::core::Polygon;
using nvision::core::PortDataType;
using nvision::core::ResourceId;
using nvision::core::ResourceIntegrity;
using nvision::core::ResourceKind;
using nvision::core::ResourceReference;
using nvision::core::SemanticVersion;
using nvision::core::Transform2D;
using nvision::acquisition::Frame;
using nvision::acquisition::FrameBuffer;
using nvision::acquisition::FrameLease;
using nvision::acquisition::FrameMetadata;
using nvision::acquisition::FrameStatus;
using nvision::acquisition::ImageView;
using nvision::acquisition::PixelFormat;
using nvision::acquisition::TimestampQuality;

template <typename Exception, typename Action>
bool rejects(Action action) {
    try {
        action();
    } catch (const Exception&) {
        return true;
    }
    return false;
}

template <typename Action>
bool rejects_invalid_argument(Action action) {
    return rejects<std::invalid_argument>(std::move(action));
}

void stable_identifiers_are_typed_and_canonical() {
    const ExecutionSessionId first("session.2026-08-25_001");
    const ExecutionSessionId same("session.2026-08-25_001");
    const ExecutionSessionId next("session.2026-08-25_002");

    NVISION_REQUIRE(first == same);
    NVISION_REQUIRE(first != next);
    NVISION_REQUIRE(first.value() == "session.2026-08-25_001");
    NVISION_REQUIRE(rejects_invalid_argument([] { ExecutionSessionId invalid(""); }));
    NVISION_REQUIRE(
        rejects_invalid_argument([] { ExecutionSessionId invalid("Session.Invalid"); }));
    NVISION_REQUIRE(
        rejects_invalid_argument([] { CoordinateSpaceId invalid("image pixel"); }));
}

void semantic_versions_validate_and_compare_precedence() {
    const SemanticVersion alpha(1U, 2U, 3U, "alpha.1", "build.9");
    const SemanticVersion alpha_next(1U, 2U, 3U, "alpha.2");
    const SemanticVersion release(1U, 2U, 3U);
    const SemanticVersion release_rebuilt(1U, 2U, 3U, "", "rebuilt.2");
    const SemanticVersion later(1U, 3U, 0U);

    NVISION_REQUIRE(alpha.to_string() == "1.2.3-alpha.1+build.9");
    NVISION_REQUIRE(alpha.compare_precedence(alpha_next) < 0);
    NVISION_REQUIRE(alpha_next.compare_precedence(release) < 0);
    NVISION_REQUIRE(release.compare_precedence(release_rebuilt) == 0);
    NVISION_REQUIRE(release.compare_precedence(later) < 0);
    NVISION_REQUIRE(rejects_invalid_argument([] {
        SemanticVersion invalid(1U, 0U, 0U, "alpha.01");
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        SemanticVersion invalid(1U, 0U, 0U, "alpha+invalid");
    }));
}

void geometry_preserves_coordinate_spaces_and_explicit_transforms() {
    const CoordinateSpaceId image_space("nvision.space.image-pixel");
    const CoordinateSpaceId roi_space("nvision.space.roi-local");
    const Point2D source(image_space, 15.0, 26.0);
    const Transform2D image_to_roi(
        image_space,
        roi_space,
        std::array<double, 9>{1.0, 0.0, -10.0, 0.0, 1.0, -20.0, 0.0, 0.0, 1.0});

    const Point2D transformed = image_to_roi.apply(source);
    NVISION_REQUIRE(transformed.coordinate_space() == roi_space);
    NVISION_REQUIRE(std::abs(transformed.x() - 5.0) < 1.0e-12);
    NVISION_REQUIRE(std::abs(transformed.y() - 6.0) < 1.0e-12);

    const Transform2D equivalently_scaled(
        image_space,
        roi_space,
        std::array<double, 9>{
            1.0e-15,
            0.0,
            -10.0e-15,
            0.0,
            1.0e-15,
            -20.0e-15,
            0.0,
            0.0,
            1.0e-15,
        });
    const Point2D scaled_result = equivalently_scaled.apply(source);
    NVISION_REQUIRE(std::abs(scaled_result.x() - 5.0) < 1.0e-12);
    NVISION_REQUIRE(std::abs(scaled_result.y() - 6.0) < 1.0e-12);

    const AxisAlignedRectangle roi(image_space, 10.0, 20.0, 100.0, 50.0);
    const Circle feature(Point2D(image_space, 50.0, 40.0), 4.0);
    const Polygon search_region(std::vector<Point2D>{
        Point2D(image_space, 0.0, 0.0),
        Point2D(image_space, 100.0, 0.0),
        Point2D(image_space, 50.0, 75.0),
    });
    NVISION_REQUIRE(roi.coordinate_space() == image_space);
    NVISION_REQUIRE(feature.coordinate_space() == image_space);
    NVISION_REQUIRE(search_region.coordinate_space() == image_space);

    NVISION_REQUIRE(rejects_invalid_argument([&] {
        Line2D invalid(Point2D(image_space, 0.0, 0.0), Point2D(roi_space, 1.0, 1.0));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        Line2D invalid(Point2D(image_space, 1.0, 1.0), Point2D(image_space, 1.0, 1.0));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        Polygon invalid(std::vector<Point2D>{
            Point2D(image_space, 0.0, 0.0),
            Point2D(image_space, 1.0, 1.0),
            Point2D(image_space, 2.0, 2.0),
        });
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        Transform2D singular(
            image_space,
            roi_space,
            std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        static_cast<void>(image_to_roi.apply(Point2D(roi_space, 0.0, 0.0)));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        Point2D invalid(image_space, std::numeric_limits<double>::infinity(), 0.0);
    }));
}

void technical_status_cannot_be_misreported_as_an_inspection_verdict() {
    const InspectionOutcome accepted(ExecutionStatus::success, InspectionVerdict::ok);
    const InspectionOutcome rejected(ExecutionStatus::success, InspectionVerdict::ng);
    const InspectionOutcome timed_out(
        ExecutionStatus::timeout,
        InspectionVerdict::indeterminate,
        "execution.timeout");

    NVISION_REQUIRE(accepted.verdict() == InspectionVerdict::ok);
    NVISION_REQUIRE(rejected.verdict() == InspectionVerdict::ng);
    NVISION_REQUIRE(timed_out.execution_status() == ExecutionStatus::timeout);
    NVISION_REQUIRE(timed_out.error_code() == "execution.timeout");
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionOutcome invalid(
            ExecutionStatus::error,
            InspectionVerdict::ok,
            "execution.failed");
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionOutcome invalid(
            ExecutionStatus::cancelled,
            InspectionVerdict::indeterminate);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionOutcome invalid(
            ExecutionStatus::error,
            InspectionVerdict::indeterminate,
            "   ");
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionOutcome invalid(
            static_cast<ExecutionStatus>(999U),
            InspectionVerdict::indeterminate,
            "execution.invalid-status");
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        InspectionOutcome invalid(
            ExecutionStatus::success,
            static_cast<InspectionVerdict>(999U));
    }));
}

void both_reference_workflows_are_expressible_with_generic_port_types() {
    constexpr std::array text_inspection_types{
        PortDataType::frame,
        PortDataType::image_view,
        PortDataType::text_result,
        PortDataType::measurement_list,
        PortDataType::defect_list,
        PortDataType::decision,
        PortDataType::overlay,
    };
    constexpr std::array point_grid_inspection_types{
        PortDataType::frame,
        PortDataType::region,
        PortDataType::feature_set,
        PortDataType::homography,
        PortDataType::point_list,
        PortDataType::point_grid_model,
        PortDataType::point_grid_result,
        PortDataType::defect_list,
        PortDataType::decision,
        PortDataType::evidence,
    };

    NVISION_REQUIRE(text_inspection_types.front() == PortDataType::frame);
    NVISION_REQUIRE(text_inspection_types.back() == PortDataType::overlay);
    NVISION_REQUIRE(point_grid_inspection_types.front() == PortDataType::frame);
    NVISION_REQUIRE(point_grid_inspection_types.back() == PortDataType::evidence);
}

void image_views_retain_frame_ownership_without_copying_pixels() {
    std::vector<std::uint8_t> bytes(48U);
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>(index);
    }

    auto buffer = std::make_shared<const FrameBuffer>(std::move(bytes));
    std::weak_ptr<const FrameBuffer> observed_buffer = buffer;
    FrameLease frame = std::make_shared<const Frame>(
        std::array<std::uint8_t, 16>{1U, 2U, 3U, 4U},
        "test-camera",
        1U,
        0,
        0,
        4U,
        3U,
        16U,
        PixelFormat::bgr8,
        buffer,
        FrameStatus::complete,
        TimestampQuality::host_only,
        FrameMetadata{});
    std::weak_ptr<const Frame> observed_frame = frame;

    {
        const ImageView view(frame, 1U, 1U, 2U, 2U);
        frame.reset();
        buffer.reset();

        NVISION_REQUIRE(!observed_frame.expired());
        NVISION_REQUIRE(!observed_buffer.expired());
        NVISION_REQUIRE(view.origin_x() == 1U);
        NVISION_REQUIRE(view.origin_y() == 1U);
        NVISION_REQUIRE(view.width() == 2U);
        NVISION_REQUIRE(view.height() == 2U);
        NVISION_REQUIRE(view.stride_bytes() == 16U);
        NVISION_REQUIRE(view.pixel_format() == PixelFormat::bgr8);

        const std::span<const std::uint8_t> first_row = view.row(0U);
        const std::span<const std::uint8_t> second_row = view.row(1U);
        NVISION_REQUIRE(first_row.size() == 6U);
        NVISION_REQUIRE(second_row.size() == 6U);
        NVISION_REQUIRE(first_row.front() == 19U);
        NVISION_REQUIRE(first_row.back() == 24U);
        NVISION_REQUIRE(second_row.front() == 35U);
        NVISION_REQUIRE(second_row.back() == 40U);
        NVISION_REQUIRE(
            first_row.data() == view.frame()->buffer()->bytes().data() + 19U);

        const ImageView nested = view.subview(1U, 0U, 1U, 2U);
        NVISION_REQUIRE(nested.origin_x() == 2U);
        NVISION_REQUIRE(nested.origin_y() == 1U);
        NVISION_REQUIRE(nested.row(0U).data() == view.frame()->buffer()->bytes().data() + 22U);
    }

    NVISION_REQUIRE(observed_frame.expired());
    NVISION_REQUIRE(observed_buffer.expired());
}

void image_views_define_frame_specific_coordinate_transforms_and_bounds() {
    auto buffer = std::make_shared<const FrameBuffer>(std::vector<std::uint8_t>(48U));
    const FrameLease frame = std::make_shared<const Frame>(
        std::array<std::uint8_t, 16>{10U, 11U, 12U, 13U},
        "test-camera",
        1U,
        0,
        0,
        4U,
        3U,
        16U,
        PixelFormat::bgr8,
        buffer,
        FrameStatus::complete,
        TimestampQuality::host_only,
        FrameMetadata{});
    const ImageView root(frame);
    const ImageView view(frame, 1U, 1U, 2U, 2U);

    NVISION_REQUIRE(root.coordinate_space() == root.source_coordinate_space());
    NVISION_REQUIRE(view.coordinate_space() != view.source_coordinate_space());
    NVISION_REQUIRE(
        view.coordinate_space().value().find("nvision.frame.") == 0U);

    const Point2D source_point = view.view_to_source().apply(
        Point2D(view.coordinate_space(), 0.0, 0.0));
    NVISION_REQUIRE(source_point.coordinate_space() == view.source_coordinate_space());
    NVISION_REQUIRE(source_point.x() == 1.0);
    NVISION_REQUIRE(source_point.y() == 1.0);

    const Point2D local_point = view.source_to_view().apply(
        Point2D(view.source_coordinate_space(), 2.0, 2.0));
    NVISION_REQUIRE(local_point.coordinate_space() == view.coordinate_space());
    NVISION_REQUIRE(local_point.x() == 1.0);
    NVISION_REQUIRE(local_point.y() == 1.0);

    NVISION_REQUIRE(rejects<std::out_of_range>([&] {
        static_cast<void>(view.row(2U));
    }));
    NVISION_REQUIRE(rejects<std::out_of_range>([&] {
        ImageView invalid(frame, 3U, 2U, 2U, 1U);
    }));
    NVISION_REQUIRE(rejects<std::out_of_range>([&] {
        static_cast<void>(view.subview(1U, 1U, 2U, 2U));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ImageView invalid(FrameLease{});
    }));

    const FrameLease unidentified = std::make_shared<const Frame>(
        std::array<std::uint8_t, 16>{},
        "test-camera",
        2U,
        0,
        0,
        4U,
        3U,
        16U,
        PixelFormat::bgr8,
        std::make_shared<const FrameBuffer>(std::vector<std::uint8_t>(48U)),
        FrameStatus::complete,
        TimestampQuality::host_only,
        FrameMetadata{});
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        ImageView invalid(unidentified);
    }));

    const FrameLease incomplete = std::make_shared<const Frame>(
        std::array<std::uint8_t, 16>{20U},
        "test-camera",
        2U,
        0,
        0,
        4U,
        3U,
        16U,
        PixelFormat::bgr8,
        std::make_shared<const FrameBuffer>(std::vector<std::uint8_t>(48U)),
        FrameStatus::incomplete,
        TimestampQuality::host_only,
        FrameMetadata{});
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        ImageView invalid(incomplete);
    }));
}

void resource_references_are_versioned_and_content_addressed() {
    constexpr std::string_view checksum =
        "3bec1d7190001d1dc1f4ef009013b1abafb1d6a7d725840f15c54920e639e313";
    const IntegrityDigest digest = IntegrityDigest::parse(std::string("sha256:") +
                                                           std::string(checksum));
    const ResourceIntegrity model_integrity(digest, 146U);
    const ResourceReference model(
        ResourceId("nvision.resource.text-detector"),
        SemanticVersion(1U, 3U, 2U),
        ResourceKind::model,
        model_integrity);
    const ResourceReference template_asset(
        ResourceId("nvision.resource.golden-template"),
        SemanticVersion(2U, 0U, 0U),
        ResourceKind::template_asset,
        ResourceIntegrity(digest, 4'096U));
    const ResourceReference lookup_resource(
        ResourceId("nvision.resource.lookup-table"),
        SemanticVersion(1U, 0U, 0U),
        ResourceKind::resource,
        ResourceIntegrity(digest, 256U));

    NVISION_REQUIRE(digest.algorithm() == IntegrityAlgorithm::sha256);
    NVISION_REQUIRE(digest.hex_digest() == checksum);
    NVISION_REQUIRE(digest.to_string() == std::string("sha256:") + std::string(checksum));
    NVISION_REQUIRE(model.id().value() == "nvision.resource.text-detector");
    NVISION_REQUIRE(model.version().to_string() == "1.3.2");
    NVISION_REQUIRE(model.kind() == ResourceKind::model);
    NVISION_REQUIRE(model.integrity().content_length_bytes() == 146U);
    NVISION_REQUIRE(model.integrity().digest() == digest);
    NVISION_REQUIRE(template_asset.kind() == ResourceKind::template_asset);
    NVISION_REQUIRE(template_asset.integrity().content_length_bytes() == 4'096U);
    NVISION_REQUIRE(lookup_resource.kind() == ResourceKind::resource);
    NVISION_REQUIRE(lookup_resource.integrity().content_length_bytes() == 256U);
}

void resource_integrity_rejects_noncanonical_or_incomplete_metadata() {
    constexpr std::string_view checksum =
        "3bec1d7190001d1dc1f4ef009013b1abafb1d6a7d725840f15c54920e639e313";
    const IntegrityDigest digest(IntegrityAlgorithm::sha256, std::string(checksum));

    NVISION_REQUIRE(rejects_invalid_argument([] {
        static_cast<void>(IntegrityDigest::parse("sha256:1234"));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        static_cast<void>(IntegrityDigest::parse(std::string("SHA256:") +
                                                 std::string(checksum)));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        std::string uppercase(checksum);
        uppercase[0] = 'A';
        IntegrityDigest invalid(IntegrityAlgorithm::sha256, std::move(uppercase));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        std::string invalid_hex(checksum);
        invalid_hex[0] = 'g';
        IntegrityDigest invalid(IntegrityAlgorithm::sha256, std::move(invalid_hex));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        IntegrityDigest invalid(static_cast<IntegrityAlgorithm>(999U), std::string(checksum));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        ResourceIntegrity invalid(digest, 0U);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&] {
        ResourceReference invalid(
            ResourceId("nvision.resource.invalid-kind"),
            SemanticVersion(1U, 0U, 0U),
            static_cast<ResourceKind>(999U),
            ResourceIntegrity(digest, 1U));
    }));
}

}  // namespace

int main() {
    stable_identifiers_are_typed_and_canonical();
    semantic_versions_validate_and_compare_precedence();
    geometry_preserves_coordinate_spaces_and_explicit_transforms();
    technical_status_cannot_be_misreported_as_an_inspection_verdict();
    both_reference_workflows_are_expressible_with_generic_port_types();
    image_views_retain_frame_ownership_without_copying_pixels();
    image_views_define_frame_specific_coordinate_transforms_and_bounds();
    resource_references_are_versioned_and_content_addressed();
    resource_integrity_rejects_noncanonical_or_incomplete_metadata();
    return 0;
}
