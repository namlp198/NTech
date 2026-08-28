#include "grid_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/measurement.hpp"
#include "nvision/core/point_grid.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace nvision::vision {

namespace {

using core::CoordinateSpaceId;
using core::Line2D;
using core::Measurement;
using core::MeasurementKey;
using core::MeasurementList;
using core::MeasurementUnit;
using core::ParameterKey;
using core::Point2D;
using core::PointGridModel;
using core::PortKey;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

double separation(const Point2D& left, const Point2D& right) noexcept {
    return std::hypot(left.x() - right.x(), left.y() - right.y());
}

class SortPointsAlongLineTool final : public VisionTool {
public:
    SortPointsAlongLineTool()
        : VisionTool("nvision.tool.structure.sort-points-along-line", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        const Line2D& line = context.input(PortKey("line")).line_value();

        if (!points.empty() && points.front().coordinate_space() != line.coordinate_space()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the points and the line are in different coordinate spaces"));
        }

        const double run = line.end().x() - line.start().x();
        const double rise = line.end().y() - line.start().y();
        const double length = std::hypot(run, rise);
        const double unit_x = run / length;
        const double unit_y = rise / length;
        const bool descending = context.parameter(ParameterKey("descending")).boolean_value();

        // Ordering by how far along the line a point projects, which is what "along a row"
        // means once the row is no longer axis aligned. The sort is stable, so points that
        // project to the same position keep the order they arrived in and a repeated run
        // produces an identical sequence.
        std::vector<Point2D> ordered = points;
        std::stable_sort(
            ordered.begin(),
            ordered.end(),
            [unit_x, unit_y, descending, &line](const Point2D& left, const Point2D& right) {
                const double left_along = (left.x() - line.start().x()) * unit_x +
                                          (left.y() - line.start().y()) * unit_y;
                const double right_along = (right.x() - line.start().x()) * unit_x +
                                           (right.y() - line.start().y()) * unit_y;
                return descending ? left_along > right_along : left_along < right_along;
            });

        PortValueMap outputs;
        outputs.set(PortKey("points"), PortValue::point_list(std::move(ordered)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class PointGridLearnTool final : public VisionTool {
public:
    PointGridLearnTool()
        : VisionTool("nvision.tool.structure.point-grid-learn", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        if (points.empty()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS", "learning a grid needs at least one point"));
        }

        const double row_tolerance =
            context.parameter(ParameterKey("row-tolerance")).floating_point_value();
        const double column_tolerance =
            context.parameter(ParameterKey("column-tolerance")).floating_point_value();

        // Points within the row tolerance of each other vertically belong to the same row.
        // Working through them in ascending vertical order means a row is closed before the
        // next one opens, so the grouping does not depend on the order they arrived in.
        std::vector<Point2D> sorted = points;
        std::stable_sort(
            sorted.begin(), sorted.end(), [](const Point2D& left, const Point2D& right) {
                return left.y() < right.y();
            });

        std::vector<std::vector<Point2D>> rows;
        for (const Point2D& point : sorted) {
            if (rows.empty() || std::abs(point.y() - rows.back().front().y()) > row_tolerance) {
                rows.push_back({point});
                continue;
            }
            rows.back().push_back(point);
        }

        for (std::vector<Point2D>& row : rows) {
            std::stable_sort(
                row.begin(), row.end(), [](const Point2D& left, const Point2D& right) {
                    return left.x() < right.x();
                });
            // Two detections closer together than the column tolerance describe one grid
            // position that was found twice, not two positions the product really has.
            row.erase(
                std::unique(
                    row.begin(),
                    row.end(),
                    [column_tolerance](const Point2D& left, const Point2D& right) {
                        return std::abs(right.x() - left.x()) <= column_tolerance;
                    }),
                row.end());
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("model"),
            PortValue::point_grid_model(
                PointGridModel(points.front().coordinate_space(), std::move(rows))));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class GridDeviationTool final : public VisionTool {
public:
    GridDeviationTool()
        : VisionTool("nvision.tool.measurement.grid-deviation", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        const PointGridModel& model = context.input(PortKey("reference")).point_grid_model_value();

        if (points.empty()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS",
                "measuring deviation needs at least one detected point to measure against"));
        }
        if (points.front().coordinate_space() != model.coordinate_space()) {
            // Deviation is a distance, so both sides have to be in the same space or the
            // number reported would not be a length at all.
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the detected points are in " + points.front().coordinate_space().value() +
                    " but the grid model is in " + model.coordinate_space().value()));
        }

        // How far each expected position sits from the nearest detection. Every expected
        // position is reported, so a caller sees the whole grid rather than only the parts
        // that happened to match well.
        const std::vector<Point2D> expected = model.points();
        MeasurementList deviations;
        deviations.reserve(expected.size());
        double largest = 0.0;
        for (std::size_t index = 0U; index < expected.size(); ++index) {
            double nearest = std::numeric_limits<double>::max();
            for (const Point2D& detected : points) {
                nearest = std::min(nearest, separation(expected[index], detected));
            }
            deviations.emplace_back(
                MeasurementKey("grid." + std::to_string(index) + ".deviation"),
                nearest,
                MeasurementUnit::pixel,
                model.coordinate_space());
            largest = std::max(largest, nearest);
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("measurements"), PortValue::measurement_list(std::move(deviations)));
        outputs.set(PortKey("maximum"), PortValue::floating_point(largest));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_sort_points_along_line_tool() {
    return std::make_shared<const SortPointsAlongLineTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_point_grid_learn_tool() {
    return std::make_shared<const PointGridLearnTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_grid_deviation_tool() {
    return std::make_shared<const GridDeviationTool>();
}

}  // namespace nvision::vision
