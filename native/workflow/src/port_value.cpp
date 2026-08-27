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

PortValue PortValue::region(core::AxisAlignedRectangle value) {
    return PortValue(PortDataType::region, std::move(value));
}

PortValue PortValue::point(core::Point2D value) {
    return PortValue(PortDataType::point_2d, std::move(value));
}

PortValue PortValue::point_list(std::vector<core::Point2D> values) {
    return PortValue(PortDataType::point_list, std::move(values));
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
