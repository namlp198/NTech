#include "nvision/workflow/parameter_schema.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nvision::workflow {

namespace {

constexpr std::size_t maximum_display_name_length = 128U;
constexpr std::size_t maximum_description_length = 4'096U;
constexpr std::size_t maximum_group_segment_length = 64U;
constexpr std::size_t maximum_unit_length = 32U;

bool is_valid(const ParameterKind kind) noexcept {
    return kind == ParameterKind::boolean || kind == ParameterKind::integer ||
           kind == ParameterKind::floating_point || kind == ParameterKind::string ||
           kind == ParameterKind::enumeration || kind == ParameterKind::array ||
           kind == ParameterKind::geometry || kind == ParameterKind::region_of_interest ||
           kind == ParameterKind::resource_reference ||
           kind == ParameterKind::model_reference ||
           kind == ParameterKind::calibration_reference;
}

bool is_valid(const GeometryKind kind) noexcept {
    return kind == GeometryKind::point || kind == GeometryKind::line ||
           kind == GeometryKind::circle || kind == GeometryKind::rectangle ||
           kind == GeometryKind::polygon;
}

bool is_roi_shape(const GeometryKind kind) noexcept {
    return kind == GeometryKind::circle || kind == GeometryKind::rectangle ||
           kind == GeometryKind::polygon;
}

bool is_valid(const PropertyEditorControl control) noexcept {
    return control == PropertyEditorControl::automatic ||
           control == PropertyEditorControl::checkbox ||
           control == PropertyEditorControl::numeric_input ||
           control == PropertyEditorControl::slider ||
           control == PropertyEditorControl::text_input ||
           control == PropertyEditorControl::multiline_text ||
           control == PropertyEditorControl::dropdown ||
           control == PropertyEditorControl::list ||
           control == PropertyEditorControl::geometry_canvas ||
           control == PropertyEditorControl::roi_canvas ||
           control == PropertyEditorControl::resource_picker;
}

bool is_valid(const VisibilityOperator operation) noexcept {
    return operation == VisibilityOperator::equals ||
           operation == VisibilityOperator::not_equals;
}

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](const unsigned char item) {
        return std::isspace(item) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char item) {
        return std::isspace(item) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

void validate_required_text(
    const std::string_view value,
    const std::size_t maximum_length,
    const std::string_view field_name) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field_name) + " must not be blank");
    }
    if (value.size() > maximum_length) {
        throw std::invalid_argument(std::string(field_name) + " exceeds its maximum length");
    }
}

GeometryKind kind_of(const GeometryValue& value) noexcept {
    if (std::holds_alternative<core::Point2D>(value)) {
        return GeometryKind::point;
    }
    if (std::holds_alternative<core::Line2D>(value)) {
        return GeometryKind::line;
    }
    if (std::holds_alternative<core::Circle>(value)) {
        return GeometryKind::circle;
    }
    if (std::holds_alternative<core::AxisAlignedRectangle>(value)) {
        return GeometryKind::rectangle;
    }
    return GeometryKind::polygon;
}

bool is_reference_kind_compatible(
    const ParameterKind parameter_kind,
    const core::ResourceKind resource_kind) noexcept {
    switch (parameter_kind) {
        case ParameterKind::resource_reference:
            return resource_kind == core::ResourceKind::resource ||
                   resource_kind == core::ResourceKind::template_asset ||
                   resource_kind == core::ResourceKind::dictionary;
        case ParameterKind::model_reference:
            return resource_kind == core::ResourceKind::model;
        case ParameterKind::calibration_reference:
            return resource_kind == core::ResourceKind::calibration;
        default:
            return false;
    }
}

bool is_editor_compatible(
    const PropertyEditorControl control,
    const ParameterKind kind) noexcept {
    switch (control) {
        case PropertyEditorControl::automatic:
            return true;
        case PropertyEditorControl::checkbox:
            return kind == ParameterKind::boolean;
        case PropertyEditorControl::numeric_input:
        case PropertyEditorControl::slider:
            return kind == ParameterKind::integer || kind == ParameterKind::floating_point;
        case PropertyEditorControl::text_input:
        case PropertyEditorControl::multiline_text:
            return kind == ParameterKind::string;
        case PropertyEditorControl::dropdown:
            return kind == ParameterKind::enumeration;
        case PropertyEditorControl::list:
            return kind == ParameterKind::array;
        case PropertyEditorControl::geometry_canvas:
            return kind == ParameterKind::geometry;
        case PropertyEditorControl::roi_canvas:
            return kind == ParameterKind::region_of_interest;
        case PropertyEditorControl::resource_picker:
            return kind == ParameterKind::resource_reference ||
                   kind == ParameterKind::model_reference ||
                   kind == ParameterKind::calibration_reference;
    }
    return false;
}

}  // namespace

ParameterValue::ParameterValue(const ParameterKind kind, Payload payload)
    : kind_(kind), payload_(std::move(payload)) {
    if (!is_valid(kind_)) {
        throw std::invalid_argument("parameter value kind is unsupported");
    }
}

ParameterValue ParameterValue::boolean(const bool value) {
    return ParameterValue(ParameterKind::boolean, value);
}

ParameterValue ParameterValue::integer(const std::int64_t value) {
    return ParameterValue(ParameterKind::integer, value);
}

ParameterValue ParameterValue::floating_point(const double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("floating-point parameter value must be finite");
    }
    return ParameterValue(ParameterKind::floating_point, value);
}

ParameterValue ParameterValue::string(std::string value) {
    return ParameterValue(ParameterKind::string, std::move(value));
}

ParameterValue ParameterValue::enumeration(core::EnumOptionKey value) {
    return ParameterValue(ParameterKind::enumeration, std::move(value));
}

ParameterValue ParameterValue::array(std::vector<ParameterValue> values) {
    return ParameterValue(
        ParameterKind::array,
        std::make_shared<const std::vector<ParameterValue>>(std::move(values)));
}

ParameterValue ParameterValue::geometry(GeometryValue value) {
    return ParameterValue(ParameterKind::geometry, std::move(value));
}

ParameterValue ParameterValue::region_of_interest(GeometryValue value) {
    if (!is_roi_shape(kind_of(value))) {
        throw std::invalid_argument("ROI parameter value must be a circle, rectangle, or polygon");
    }
    return ParameterValue(ParameterKind::region_of_interest, std::move(value));
}

ParameterValue ParameterValue::resource_reference(core::ResourceReference value) {
    if (!is_reference_kind_compatible(ParameterKind::resource_reference, value.kind())) {
        throw std::invalid_argument("generic resource parameter contains an incompatible kind");
    }
    return ParameterValue(ParameterKind::resource_reference, std::move(value));
}

ParameterValue ParameterValue::model_reference(core::ResourceReference value) {
    if (!is_reference_kind_compatible(ParameterKind::model_reference, value.kind())) {
        throw std::invalid_argument("model parameter requires a model reference");
    }
    return ParameterValue(ParameterKind::model_reference, std::move(value));
}

ParameterValue ParameterValue::calibration_reference(core::ResourceReference value) {
    if (!is_reference_kind_compatible(ParameterKind::calibration_reference, value.kind())) {
        throw std::invalid_argument("calibration parameter requires a calibration reference");
    }
    return ParameterValue(ParameterKind::calibration_reference, std::move(value));
}

ParameterKind ParameterValue::kind() const noexcept {
    return kind_;
}

bool ParameterValue::boolean_value() const {
    return std::get<bool>(payload_);
}

std::int64_t ParameterValue::integer_value() const {
    return std::get<std::int64_t>(payload_);
}

double ParameterValue::floating_point_value() const {
    return std::get<double>(payload_);
}

const std::string& ParameterValue::string_value() const {
    return std::get<std::string>(payload_);
}

const core::EnumOptionKey& ParameterValue::enumeration_value() const {
    return std::get<core::EnumOptionKey>(payload_);
}

const std::vector<ParameterValue>& ParameterValue::array_value() const {
    return *std::get<ArrayStorage>(payload_);
}

const GeometryValue& ParameterValue::geometry_value() const {
    return std::get<GeometryValue>(payload_);
}

GeometryKind ParameterValue::geometry_kind() const {
    return kind_of(geometry_value());
}

const core::ResourceReference& ParameterValue::reference_value() const {
    return std::get<core::ResourceReference>(payload_);
}

NumericConstraint::NumericConstraint(
    const std::optional<double> minimum,
    const std::optional<double> maximum,
    const std::optional<double> step)
    : minimum_(minimum), maximum_(maximum), step_(step) {
    const auto finite = [](const std::optional<double>& value) {
        return !value || std::isfinite(*value);
    };
    if (!finite(minimum_) || !finite(maximum_) || !finite(step_)) {
        throw std::invalid_argument("numeric constraints must be finite");
    }
    if (minimum_ && maximum_ && *minimum_ > *maximum_) {
        throw std::invalid_argument("numeric minimum must not exceed maximum");
    }
    if (step_ && *step_ <= 0.0) {
        throw std::invalid_argument("numeric step must be positive");
    }
}

const std::optional<double>& NumericConstraint::minimum() const noexcept {
    return minimum_;
}

const std::optional<double>& NumericConstraint::maximum() const noexcept {
    return maximum_;
}

const std::optional<double>& NumericConstraint::step() const noexcept {
    return step_;
}

bool NumericConstraint::accepts(const double value) const noexcept {
    return std::isfinite(value) && (!minimum_ || value >= *minimum_) &&
           (!maximum_ || value <= *maximum_);
}

EnumOption::EnumOption(
    core::EnumOptionKey key,
    std::string display_name,
    std::string description)
    : key_(std::move(key)),
      display_name_(trim(std::move(display_name))),
      description_(trim(std::move(description))) {
    validate_required_text(display_name_, maximum_display_name_length, "enum display name");
    if (description_.size() > maximum_description_length) {
        throw std::invalid_argument("enum description exceeds its maximum length");
    }
}

const core::EnumOptionKey& EnumOption::key() const noexcept {
    return key_;
}

const std::string& EnumOption::display_name() const noexcept {
    return display_name_;
}

const std::string& EnumOption::description() const noexcept {
    return description_;
}

ParameterType::ParameterType(const ParameterKind kind) : kind_(kind) {
    if (!is_valid(kind_)) {
        throw std::invalid_argument("parameter type kind is unsupported");
    }
}

ParameterType ParameterType::boolean() {
    return ParameterType(ParameterKind::boolean);
}

ParameterType ParameterType::integer(NumericConstraint constraint) {
    ParameterType result(ParameterKind::integer);
    result.numeric_constraint_ = std::move(constraint);
    return result;
}

ParameterType ParameterType::floating_point(NumericConstraint constraint) {
    ParameterType result(ParameterKind::floating_point);
    result.numeric_constraint_ = std::move(constraint);
    return result;
}

ParameterType ParameterType::string() {
    return ParameterType(ParameterKind::string);
}

ParameterType ParameterType::enumeration(std::vector<EnumOption> options) {
    if (options.empty()) {
        throw std::invalid_argument("enumeration parameter requires at least one option");
    }
    for (std::size_t index = 0U; index < options.size(); ++index) {
        const auto duplicate = std::find_if(
            options.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            options.end(),
            [&options, index](const EnumOption& candidate) {
                return candidate.key() == options[index].key();
            });
        if (duplicate != options.end()) {
            throw std::invalid_argument("enumeration option keys must be unique");
        }
    }
    ParameterType result(ParameterKind::enumeration);
    result.enum_options_ = std::move(options);
    return result;
}

ParameterType ParameterType::array(
    ParameterType element_type,
    const std::size_t minimum_items,
    const std::optional<std::size_t> maximum_items) {
    if (maximum_items && minimum_items > *maximum_items) {
        throw std::invalid_argument("array minimum items must not exceed maximum items");
    }
    ParameterType result(ParameterKind::array);
    result.element_type_ = std::make_shared<const ParameterType>(std::move(element_type));
    result.minimum_items_ = minimum_items;
    result.maximum_items_ = maximum_items;
    return result;
}

ParameterType ParameterType::geometry(const GeometryKind geometry_kind) {
    if (!is_valid(geometry_kind)) {
        throw std::invalid_argument("geometry parameter kind is unsupported");
    }
    ParameterType result(ParameterKind::geometry);
    result.geometry_kind_ = geometry_kind;
    return result;
}

ParameterType ParameterType::region_of_interest(std::vector<GeometryKind> allowed_shapes) {
    if (allowed_shapes.empty()) {
        throw std::invalid_argument("ROI parameter requires at least one allowed shape");
    }
    if (std::any_of(allowed_shapes.begin(), allowed_shapes.end(), [](const GeometryKind kind) {
            return !is_roi_shape(kind);
        })) {
        throw std::invalid_argument("ROI supports only circle, rectangle, or polygon shapes");
    }
    std::vector<GeometryKind> unique_shapes = allowed_shapes;
    std::sort(unique_shapes.begin(), unique_shapes.end());
    if (std::adjacent_find(unique_shapes.begin(), unique_shapes.end()) != unique_shapes.end()) {
        throw std::invalid_argument("ROI shape kinds must be unique");
    }
    ParameterType result(ParameterKind::region_of_interest);
    result.allowed_roi_shapes_ = std::move(allowed_shapes);
    return result;
}

ParameterType ParameterType::resource_reference() {
    return ParameterType(ParameterKind::resource_reference);
}

ParameterType ParameterType::model_reference() {
    return ParameterType(ParameterKind::model_reference);
}

ParameterType ParameterType::calibration_reference() {
    return ParameterType(ParameterKind::calibration_reference);
}

ParameterKind ParameterType::kind() const noexcept {
    return kind_;
}

const std::optional<NumericConstraint>& ParameterType::numeric_constraint() const noexcept {
    return numeric_constraint_;
}

const std::vector<EnumOption>& ParameterType::enum_options() const noexcept {
    return enum_options_;
}

const std::shared_ptr<const ParameterType>& ParameterType::element_type() const noexcept {
    return element_type_;
}

std::size_t ParameterType::minimum_items() const noexcept {
    return minimum_items_;
}

const std::optional<std::size_t>& ParameterType::maximum_items() const noexcept {
    return maximum_items_;
}

const std::optional<GeometryKind>& ParameterType::geometry_kind() const noexcept {
    return geometry_kind_;
}

const std::vector<GeometryKind>& ParameterType::allowed_roi_shapes() const noexcept {
    return allowed_roi_shapes_;
}

bool ParameterType::accepts(const ParameterValue& value) const noexcept {
    if (value.kind() != kind_) {
        return false;
    }
    switch (kind_) {
        case ParameterKind::boolean:
        case ParameterKind::string:
            return true;
        case ParameterKind::integer:
            return numeric_constraint_->accepts(static_cast<double>(value.integer_value()));
        case ParameterKind::floating_point:
            return numeric_constraint_->accepts(value.floating_point_value());
        case ParameterKind::enumeration:
            return std::any_of(
                enum_options_.begin(),
                enum_options_.end(),
                [&value](const EnumOption& option) {
                    return option.key() == value.enumeration_value();
                });
        case ParameterKind::array: {
            const auto& items = value.array_value();
            return items.size() >= minimum_items_ &&
                   (!maximum_items_ || items.size() <= *maximum_items_) &&
                   std::all_of(items.begin(), items.end(), [this](const ParameterValue& item) {
                       return element_type_->accepts(item);
                   });
        }
        case ParameterKind::geometry:
            return value.geometry_kind() == *geometry_kind_;
        case ParameterKind::region_of_interest:
            return std::find(
                       allowed_roi_shapes_.begin(),
                       allowed_roi_shapes_.end(),
                       value.geometry_kind()) != allowed_roi_shapes_.end();
        case ParameterKind::resource_reference:
        case ParameterKind::model_reference:
        case ParameterKind::calibration_reference:
            return is_reference_kind_compatible(kind_, value.reference_value().kind());
    }
    return false;
}

namespace {

// A numeric domain is contained when it is bounded at least everywhere the target is, and
// no wider on either side. An unbounded source can never fit inside a bounded target.
bool numeric_domain_fits(const NumericConstraint& source, const NumericConstraint& target) {
    if (target.minimum().has_value() &&
        (!source.minimum().has_value() || *source.minimum() < *target.minimum())) {
        return false;
    }
    if (target.maximum().has_value() &&
        (!source.maximum().has_value() || *source.maximum() > *target.maximum())) {
        return false;
    }
    if (target.step().has_value() &&
        (!source.step().has_value() || *source.step() != *target.step())) {
        // A coarser grid can still sit off the target's, and proving alignment needs the
        // origin as well as the spacing. Requiring the same step refuses some assignments
        // that would work rather than admitting one that would not.
        return false;
    }
    return true;
}

}  // namespace

bool ParameterType::is_assignable_to(const ParameterType& target) const noexcept {
    if (kind_ != target.kind_) {
        return false;
    }
    switch (kind_) {
        case ParameterKind::boolean:
        case ParameterKind::string:
            return true;
        case ParameterKind::integer:
        case ParameterKind::floating_point:
            return numeric_domain_fits(*numeric_constraint_, *target.numeric_constraint_);
        case ParameterKind::enumeration:
            return std::all_of(
                enum_options_.begin(),
                enum_options_.end(),
                [&target](const EnumOption& option) {
                    return std::any_of(
                        target.enum_options_.begin(),
                        target.enum_options_.end(),
                        [&option](const EnumOption& allowed) {
                            return allowed.key() == option.key();
                        });
                });
        case ParameterKind::array:
            if (!element_type_->is_assignable_to(*target.element_type_)) {
                return false;
            }
            if (minimum_items_ < target.minimum_items_) {
                return false;
            }
            return !target.maximum_items_.has_value() ||
                   (maximum_items_.has_value() && *maximum_items_ <= *target.maximum_items_);
        case ParameterKind::geometry:
            return *geometry_kind_ == *target.geometry_kind_;
        case ParameterKind::region_of_interest:
            return std::all_of(
                allowed_roi_shapes_.begin(),
                allowed_roi_shapes_.end(),
                [&target](const GeometryKind shape) {
                    return std::find(
                               target.allowed_roi_shapes_.begin(),
                               target.allowed_roi_shapes_.end(),
                               shape) != target.allowed_roi_shapes_.end();
                });
        case ParameterKind::resource_reference:
        case ParameterKind::model_reference:
        case ParameterKind::calibration_reference:
            // The kinds already match, and a reference carries no further domain.
            return true;
    }
    return false;
}

PropertyEditorMetadata::PropertyEditorMetadata(
    std::vector<std::string> group_path,
    const std::uint32_t order,
    const PropertyEditorControl control,
    std::string unit)
    : group_path_(std::move(group_path)),
      order_(order),
      control_(control),
      unit_(trim(std::move(unit))) {
    if (!is_valid(control_)) {
        throw std::invalid_argument("property editor control is unsupported");
    }
    for (auto& segment : group_path_) {
        segment = trim(std::move(segment));
        validate_required_text(segment, maximum_group_segment_length, "property group segment");
        if (segment.find('/') != std::string::npos) {
            throw std::invalid_argument("property group segments must not contain '/'");
        }
    }
    if (unit_.size() > maximum_unit_length) {
        throw std::invalid_argument("property unit exceeds its maximum length");
    }
}

const std::vector<std::string>& PropertyEditorMetadata::group_path() const noexcept {
    return group_path_;
}

std::uint32_t PropertyEditorMetadata::order() const noexcept {
    return order_;
}

PropertyEditorControl PropertyEditorMetadata::control() const noexcept {
    return control_;
}

const std::string& PropertyEditorMetadata::unit() const noexcept {
    return unit_;
}

VisibilityCondition::VisibilityCondition(
    core::ParameterKey controller,
    const VisibilityOperator operation,
    ParameterValue expected_value)
    : controller_(std::move(controller)),
      operation_(operation),
      expected_value_(std::move(expected_value)) {
    if (!is_valid(operation_)) {
        throw std::invalid_argument("visibility operation is unsupported");
    }
}

const core::ParameterKey& VisibilityCondition::controller() const noexcept {
    return controller_;
}

VisibilityOperator VisibilityCondition::operation() const noexcept {
    return operation_;
}

const ParameterValue& VisibilityCondition::expected_value() const noexcept {
    return expected_value_;
}

ParameterDefinition::ParameterDefinition(
    core::ParameterKey key,
    std::string display_name,
    ParameterType type,
    const bool required,
    std::optional<ParameterValue> default_value,
    std::string description,
    PropertyEditorMetadata editor,
    std::optional<VisibilityCondition> visibility,
    const bool recipe_overridable,
    const bool runtime_bindable)
    : key_(std::move(key)),
      display_name_(trim(std::move(display_name))),
      type_(std::move(type)),
      required_(required),
      default_value_(std::move(default_value)),
      description_(trim(std::move(description))),
      editor_(std::move(editor)),
      visibility_(std::move(visibility)),
      recipe_overridable_(recipe_overridable),
      runtime_bindable_(runtime_bindable) {
    validate_required_text(display_name_, maximum_display_name_length, "parameter display name");
    if (description_.size() > maximum_description_length) {
        throw std::invalid_argument("parameter description exceeds its maximum length");
    }
    if (default_value_ && !type_.accepts(*default_value_)) {
        throw std::invalid_argument("parameter default value does not satisfy its type schema");
    }
    if (!is_editor_compatible(editor_.control(), type_.kind())) {
        throw std::invalid_argument("property editor control is incompatible with parameter type");
    }
    if (!editor_.unit().empty() && type_.kind() != ParameterKind::integer &&
        type_.kind() != ParameterKind::floating_point) {
        throw std::invalid_argument("only numeric parameters may declare a unit");
    }
}

const core::ParameterKey& ParameterDefinition::key() const noexcept {
    return key_;
}

const std::string& ParameterDefinition::display_name() const noexcept {
    return display_name_;
}

const ParameterType& ParameterDefinition::type() const noexcept {
    return type_;
}

bool ParameterDefinition::required() const noexcept {
    return required_;
}

const std::optional<ParameterValue>& ParameterDefinition::default_value() const noexcept {
    return default_value_;
}

const std::string& ParameterDefinition::description() const noexcept {
    return description_;
}

const PropertyEditorMetadata& ParameterDefinition::editor() const noexcept {
    return editor_;
}

const std::optional<VisibilityCondition>& ParameterDefinition::visibility() const noexcept {
    return visibility_;
}

bool ParameterDefinition::recipe_overridable() const noexcept {
    return recipe_overridable_;
}

bool ParameterDefinition::runtime_bindable() const noexcept {
    return runtime_bindable_;
}

}  // namespace nvision::workflow
