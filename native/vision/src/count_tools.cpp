#include "count_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/detection.hpp"
#include "nvision/core/measurement.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::Circle;
using core::Measurement;
using core::MeasurementKey;
using core::MeasurementList;
using core::MeasurementUnit;
using core::Point2D;
using core::PortKey;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

class PixelCountTool final : public VisionTool {
public:
    PixelCountTool()
        : VisionTool("nvision.tool.measurement.pixel-count", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& mask = context.input(PortKey("mask")).image_value();
        if (mask.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT", "counting selected pixels requires an 8-bit mask"));
        }

        const auto selected = static_cast<std::int64_t>(cv::countNonZero(interop::borrow(mask)));
        const auto total = static_cast<double>(mask.width()) * static_cast<double>(mask.height());

        PortValueMap outputs;
        outputs.set(PortKey("count"), PortValue::integer(selected));
        outputs.set(
            PortKey("ratio"),
            PortValue::floating_point(static_cast<double>(selected) / total));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class ObjectCountTool final : public VisionTool {
public:
    ObjectCountTool()
        : VisionTool("nvision.tool.measurement.object-count", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& objects = context.input(PortKey("objects")).detection_list_value();

        PortValueMap outputs;
        outputs.set(
            PortKey("count"), PortValue::integer(static_cast<std::int64_t>(objects.size())));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class DistanceTool final : public VisionTool {
public:
    DistanceTool() : VisionTool("nvision.tool.measurement.distance", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const Point2D& from = context.input(PortKey("from")).point_value();
        const Point2D& to = context.input(PortKey("to")).point_value();

        if (from.coordinate_space() != to.coordinate_space()) {
            // A distance between two spaces is not a length, it is a category error.
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the two points are in different coordinate spaces, so their separation is "
                "not a distance"));
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("measurement"),
            PortValue::measurement(Measurement(
                MeasurementKey("distance"),
                std::hypot(to.x() - from.x(), to.y() - from.y()),
                MeasurementUnit::pixel,
                from.coordinate_space())));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class RadiusDiameterTool final : public VisionTool {
public:
    RadiusDiameterTool()
        : VisionTool("nvision.tool.measurement.radius-diameter", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const Circle& circle = context.input(PortKey("circle")).circle_value();

        MeasurementList measurements;
        measurements.emplace_back(
            MeasurementKey("circle.radius"),
            circle.radius(),
            MeasurementUnit::pixel,
            circle.coordinate_space());
        measurements.emplace_back(
            MeasurementKey("circle.diameter"),
            circle.radius() * 2.0,
            MeasurementUnit::pixel,
            circle.coordinate_space());

        PortValueMap outputs;
        outputs.set(
            PortKey("measurements"), PortValue::measurement_list(std::move(measurements)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_pixel_count_tool() {
    return std::make_shared<const PixelCountTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_object_count_tool() {
    return std::make_shared<const ObjectCountTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_distance_tool() {
    return std::make_shared<const DistanceTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_radius_diameter_tool() {
    return std::make_shared<const RadiusDiameterTool>();
}

}  // namespace nvision::vision
