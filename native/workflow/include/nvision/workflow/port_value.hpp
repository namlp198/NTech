#pragma once

#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/data_type.hpp"
#include "nvision/core/detection.hpp"
#include "nvision/core/execution_result.hpp"
#include "nvision/core/feature.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/measurement.hpp"
#include "nvision/core/point_grid.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace nvision::workflow {

// One typed value flowing across a workflow port during execution.
//
// The declared PortDataType is the contract a tool definition validates against, and
// the payload is the native representation agreed for that kind. Both are set by the
// same factory so a value can never claim a kind its payload does not carry. Kinds
// whose native representation arrives in a later milestone have no factory yet.
class PortValue final {
public:
    [[nodiscard]] static PortValue image(acquisition::ImageView value);
    [[nodiscard]] static PortValue mask(acquisition::ImageView value);
    [[nodiscard]] static PortValue transform(core::Transform2D value);
    // A homography is carried as a transform because it is one: a 3x3 mapping with an
    // explicit source and target coordinate space. The port kind records that the mapping
    // is projective rather than affine.
    [[nodiscard]] static PortValue homography(core::Transform2D value);
    [[nodiscard]] static PortValue region(core::AxisAlignedRectangle value);
    [[nodiscard]] static PortValue point(core::Point2D value);
    [[nodiscard]] static PortValue point_list(std::vector<core::Point2D> values);
    [[nodiscard]] static PortValue line(core::Line2D value);
    [[nodiscard]] static PortValue circle(core::Circle value);
    [[nodiscard]] static PortValue contour(core::Contour value);
    [[nodiscard]] static PortValue contour_list(std::vector<core::Contour> values);
    [[nodiscard]] static PortValue detection(core::Detection value);
    [[nodiscard]] static PortValue detection_list(core::DetectionList values);
    [[nodiscard]] static PortValue measurement(core::Measurement value);
    [[nodiscard]] static PortValue measurement_list(core::MeasurementList values);
    [[nodiscard]] static PortValue feature_set(core::FeatureSet value);
    [[nodiscard]] static PortValue match_set(core::MatchSet value);
    [[nodiscard]] static PortValue point_grid_model(core::PointGridModel value);
    [[nodiscard]] static PortValue boolean(bool value);
    [[nodiscard]] static PortValue integer(std::int64_t value);
    [[nodiscard]] static PortValue floating_point(double value);
    [[nodiscard]] static PortValue text(std::string value);
    [[nodiscard]] static PortValue decision(core::InspectionDecision value);

    [[nodiscard]] core::PortDataType type() const noexcept;

    [[nodiscard]] const acquisition::ImageView& image_value() const;
    [[nodiscard]] const core::Transform2D& transform_value() const;
    [[nodiscard]] const core::AxisAlignedRectangle& region_value() const;
    [[nodiscard]] const core::Point2D& point_value() const;
    [[nodiscard]] const std::vector<core::Point2D>& point_list_value() const;
    [[nodiscard]] const core::Line2D& line_value() const;
    [[nodiscard]] const core::Circle& circle_value() const;
    [[nodiscard]] const core::Contour& contour_value() const;
    [[nodiscard]] const std::vector<core::Contour>& contour_list_value() const;
    [[nodiscard]] const core::Detection& detection_value() const;
    [[nodiscard]] const core::DetectionList& detection_list_value() const;
    [[nodiscard]] const core::Measurement& measurement_value() const;
    [[nodiscard]] const core::MeasurementList& measurement_list_value() const;
    [[nodiscard]] const core::FeatureSet& feature_set_value() const;
    [[nodiscard]] const core::MatchSet& match_set_value() const;
    [[nodiscard]] const core::PointGridModel& point_grid_model_value() const;
    [[nodiscard]] bool boolean_value() const;
    [[nodiscard]] std::int64_t integer_value() const;
    [[nodiscard]] double floating_point_value() const;
    [[nodiscard]] const std::string& text_value() const;
    [[nodiscard]] const core::InspectionDecision& decision_value() const;

private:
    using Payload = std::variant<
        bool,
        std::int64_t,
        double,
        std::string,
        acquisition::ImageView,
        core::Transform2D,
        core::AxisAlignedRectangle,
        core::Point2D,
        std::vector<core::Point2D>,
        core::Line2D,
        core::Circle,
        core::Contour,
        std::vector<core::Contour>,
        core::Detection,
        core::DetectionList,
        core::Measurement,
        core::MeasurementList,
        core::FeatureSet,
        core::MatchSet,
        core::PointGridModel,
        core::InspectionDecision>;

    PortValue(core::PortDataType type, Payload payload);

    core::PortDataType type_;
    Payload payload_;
};

// Port values keyed by port key, in the order they were supplied. Ordering is kept so
// that traces and evidence stay reproducible across runs.
class PortValueMap final {
public:
    using Entry = std::pair<core::PortKey, PortValue>;

    PortValueMap() = default;
    explicit PortValueMap(std::vector<Entry> entries);

    void set(core::PortKey key, PortValue value);
    [[nodiscard]] const PortValue* find(const core::PortKey& key) const noexcept;
    [[nodiscard]] bool contains(const core::PortKey& key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<Entry>& entries() const noexcept;

private:
    std::vector<Entry> entries_;
};

}  // namespace nvision::workflow
