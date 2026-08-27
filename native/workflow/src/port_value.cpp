#include "nvision/workflow/port_value.hpp"

#include <algorithm>
#include <stdexcept>

namespace nvision::workflow {

namespace {

using core::PortDataType;

[[noreturn]] void reject_payload_access(const char* const requested_kind) {
    throw std::invalid_argument(
        std::string("port value does not carry a ") + requested_kind + " payload");
}

template <typename Payload, typename Storage>
const Payload& payload_of(const Storage& storage, const char* const requested_kind) {
    const Payload* value = std::get_if<Payload>(&storage);
    if (value == nullptr) {
        reject_payload_access(requested_kind);
    }
    return *value;
}

}  // namespace

PortValue::PortValue(const PortDataType type, Payload payload)
    : type_(type), payload_(std::move(payload)) {}

PortValue PortValue::image(acquisition::ImageView value) {
    return PortValue(PortDataType::image_view, std::move(value));
}

PortValue PortValue::mask(acquisition::ImageView value) {
    return PortValue(PortDataType::mask, std::move(value));
}

PortValue PortValue::transform(core::Transform2D value) {
    return PortValue(PortDataType::transform, std::move(value));
}

PortValue PortValue::homography(core::Transform2D value) {
    return PortValue(PortDataType::homography, std::move(value));
}

PortValue PortValue::region(core::AxisAlignedRectangle value) {
    return PortValue(PortDataType::region, std::move(value));
}

PortValue PortValue::point(core::Point2D value) {
    return PortValue(PortDataType::point_2d, std::move(value));
}

PortValue PortValue::point_list(std::vector<core::Point2D> values) {
    // Points that do not share a space cannot be compared, fitted, or matched against one
    // another, so a mixed list is rejected where it is built rather than where it is used.
    if (!values.empty()) {
        const core::CoordinateSpaceId& space = values.front().coordinate_space();
        if (!std::all_of(values.begin(), values.end(), [&space](const core::Point2D& point) {
                return point.coordinate_space() == space;
            })) {
            throw std::invalid_argument("a point list must use one coordinate space");
        }
    }
    return PortValue(PortDataType::point_list, std::move(values));
}

PortValue PortValue::line(core::Line2D value) {
    return PortValue(PortDataType::line_2d, std::move(value));
}

PortValue PortValue::circle(core::Circle value) {
    return PortValue(PortDataType::circle, std::move(value));
}

PortValue PortValue::contour(core::Contour value) {
    return PortValue(PortDataType::contour, std::move(value));
}

PortValue PortValue::contour_list(std::vector<core::Contour> values) {
    return PortValue(PortDataType::contour_list, std::move(values));
}

PortValue PortValue::detection(core::Detection value) {
    return PortValue(PortDataType::detection, std::move(value));
}

PortValue PortValue::detection_list(core::DetectionList values) {
    return PortValue(PortDataType::detection_list, std::move(values));
}

PortValue PortValue::measurement(core::Measurement value) {
    return PortValue(PortDataType::measurement, std::move(value));
}

PortValue PortValue::measurement_list(core::MeasurementList values) {
    return PortValue(PortDataType::measurement_list, std::move(values));
}

PortValue PortValue::feature_set(core::FeatureSet value) {
    return PortValue(PortDataType::feature_set, std::move(value));
}

PortValue PortValue::match_set(core::MatchSet value) {
    return PortValue(PortDataType::match_set, std::move(value));
}

PortValue PortValue::point_grid_model(core::PointGridModel value) {
    return PortValue(PortDataType::point_grid_model, std::move(value));
}

PortValue PortValue::boolean(const bool value) {
    return PortValue(PortDataType::boolean, value);
}

PortValue PortValue::integer(const std::int64_t value) {
    return PortValue(PortDataType::integer, value);
}

PortValue PortValue::floating_point(const double value) {
    return PortValue(PortDataType::floating_point, value);
}

PortValue PortValue::text(std::string value) {
    return PortValue(PortDataType::string, std::move(value));
}

PortValue PortValue::decision(core::InspectionDecision value) {
    return PortValue(PortDataType::decision, std::move(value));
}

PortDataType PortValue::type() const noexcept {
    return type_;
}

const acquisition::ImageView& PortValue::image_value() const {
    return payload_of<acquisition::ImageView>(payload_, "image");
}

const core::Transform2D& PortValue::transform_value() const {
    return payload_of<core::Transform2D>(payload_, "transform");
}

const core::AxisAlignedRectangle& PortValue::region_value() const {
    return payload_of<core::AxisAlignedRectangle>(payload_, "region");
}

const core::Point2D& PortValue::point_value() const {
    return payload_of<core::Point2D>(payload_, "point");
}

const std::vector<core::Point2D>& PortValue::point_list_value() const {
    return payload_of<std::vector<core::Point2D>>(payload_, "point list");
}

bool PortValue::boolean_value() const {
    return payload_of<bool>(payload_, "boolean");
}

std::int64_t PortValue::integer_value() const {
    return payload_of<std::int64_t>(payload_, "integer");
}

double PortValue::floating_point_value() const {
    return payload_of<double>(payload_, "floating point");
}

const std::string& PortValue::text_value() const {
    return payload_of<std::string>(payload_, "text");
}

const core::Line2D& PortValue::line_value() const {
    return payload_of<core::Line2D>(payload_, "line");
}

const core::Circle& PortValue::circle_value() const {
    return payload_of<core::Circle>(payload_, "circle");
}

const core::Contour& PortValue::contour_value() const {
    return payload_of<core::Contour>(payload_, "contour");
}

const std::vector<core::Contour>& PortValue::contour_list_value() const {
    return payload_of<std::vector<core::Contour>>(payload_, "contour list");
}

const core::Detection& PortValue::detection_value() const {
    return payload_of<core::Detection>(payload_, "detection");
}

const core::DetectionList& PortValue::detection_list_value() const {
    return payload_of<core::DetectionList>(payload_, "detection list");
}

const core::Measurement& PortValue::measurement_value() const {
    return payload_of<core::Measurement>(payload_, "measurement");
}

const core::MeasurementList& PortValue::measurement_list_value() const {
    return payload_of<core::MeasurementList>(payload_, "measurement list");
}

const core::FeatureSet& PortValue::feature_set_value() const {
    return payload_of<core::FeatureSet>(payload_, "feature set");
}

const core::MatchSet& PortValue::match_set_value() const {
    return payload_of<core::MatchSet>(payload_, "match set");
}

const core::PointGridModel& PortValue::point_grid_model_value() const {
    return payload_of<core::PointGridModel>(payload_, "point grid model");
}

const core::InspectionDecision& PortValue::decision_value() const {
    return payload_of<core::InspectionDecision>(payload_, "decision");
}

PortValueMap::PortValueMap(std::vector<Entry> entries) : entries_(std::move(entries)) {
    for (std::size_t index = 0U; index < entries_.size(); ++index) {
        const auto duplicate = std::find_if(
            entries_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            entries_.end(),
            [this, index](const Entry& candidate) {
                return candidate.first == entries_[index].first;
            });
        if (duplicate != entries_.end()) {
            throw std::invalid_argument("port value keys must be unique");
        }
    }
}

void PortValueMap::set(core::PortKey key, PortValue value) {
    const auto existing = std::find_if(
        entries_.begin(), entries_.end(), [&key](const Entry& candidate) {
            return candidate.first == key;
        });
    if (existing != entries_.end()) {
        existing->second = std::move(value);
        return;
    }
    entries_.emplace_back(std::move(key), std::move(value));
}

const PortValue* PortValueMap::find(const core::PortKey& key) const noexcept {
    const auto match = std::find_if(
        entries_.begin(), entries_.end(), [&key](const Entry& candidate) {
            return candidate.first == key;
        });
    return match == entries_.end() ? nullptr : &match->second;
}

bool PortValueMap::contains(const core::PortKey& key) const noexcept {
    return find(key) != nullptr;
}

std::size_t PortValueMap::size() const noexcept {
    return entries_.size();
}

bool PortValueMap::empty() const noexcept {
    return entries_.empty();
}

const std::vector<PortValueMap::Entry>& PortValueMap::entries() const noexcept {
    return entries_;
}

}  // namespace nvision::workflow
