#include "nvision/core/data_type.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/core/inspection_outcome.hpp"
#include "nvision/core/semantic_version.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
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

using nvision::core::AxisAlignedRectangle;
using nvision::core::Circle;
using nvision::core::CoordinateSpaceId;
using nvision::core::ExecutionSessionId;
using nvision::core::ExecutionStatus;
using nvision::core::InspectionOutcome;
using nvision::core::InspectionVerdict;
using nvision::core::Line2D;
using nvision::core::Point2D;
using nvision::core::Polygon;
using nvision::core::PortDataType;
using nvision::core::SemanticVersion;
using nvision::core::Transform2D;

template <typename Action>
bool rejects_invalid_argument(Action action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
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

}  // namespace

int main() {
    stable_identifiers_are_typed_and_canonical();
    semantic_versions_validate_and_compare_precedence();
    geometry_preserves_coordinate_spaces_and_explicit_transforms();
    technical_status_cannot_be_misreported_as_an_inspection_verdict();
    both_reference_workflows_are_expressible_with_generic_port_types();
    return 0;
}
