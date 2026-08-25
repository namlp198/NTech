#pragma once

#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/resource_reference.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace nvision::core {

enum class ParameterKind : std::uint32_t {
    boolean = 0,
    integer = 1,
    floating_point = 2,
    string = 3,
    enumeration = 4,
    array = 5,
    geometry = 6,
    region_of_interest = 7,
    resource_reference = 8,
    model_reference = 9,
    calibration_reference = 10,
};

enum class GeometryKind : std::uint32_t {
    point = 0,
    line = 1,
    circle = 2,
    rectangle = 3,
    polygon = 4,
};

using GeometryValue =
    std::variant<Point2D, Line2D, Circle, AxisAlignedRectangle, Polygon>;

class ParameterValue final {
public:
    [[nodiscard]] static ParameterValue boolean(bool value);
    [[nodiscard]] static ParameterValue integer(std::int64_t value);
    [[nodiscard]] static ParameterValue floating_point(double value);
    [[nodiscard]] static ParameterValue string(std::string value);
    [[nodiscard]] static ParameterValue enumeration(EnumOptionKey value);
    [[nodiscard]] static ParameterValue array(std::vector<ParameterValue> values);
    [[nodiscard]] static ParameterValue geometry(GeometryValue value);
    [[nodiscard]] static ParameterValue region_of_interest(GeometryValue value);
    [[nodiscard]] static ParameterValue resource_reference(ResourceReference value);
    [[nodiscard]] static ParameterValue model_reference(ResourceReference value);
    [[nodiscard]] static ParameterValue calibration_reference(ResourceReference value);

    [[nodiscard]] ParameterKind kind() const noexcept;
    [[nodiscard]] bool boolean_value() const;
    [[nodiscard]] std::int64_t integer_value() const;
    [[nodiscard]] double floating_point_value() const;
    [[nodiscard]] const std::string& string_value() const;
    [[nodiscard]] const EnumOptionKey& enumeration_value() const;
    [[nodiscard]] const std::vector<ParameterValue>& array_value() const;
    [[nodiscard]] const GeometryValue& geometry_value() const;
    [[nodiscard]] GeometryKind geometry_kind() const;
    [[nodiscard]] const ResourceReference& reference_value() const;

private:
    using ArrayStorage = std::shared_ptr<const std::vector<ParameterValue>>;
    using Payload = std::variant<
        bool,
        std::int64_t,
        double,
        std::string,
        EnumOptionKey,
        ArrayStorage,
        GeometryValue,
        ResourceReference>;

    ParameterValue(ParameterKind kind, Payload payload);

    ParameterKind kind_;
    Payload payload_;
};

class NumericConstraint final {
public:
    NumericConstraint(
        std::optional<double> minimum = std::nullopt,
        std::optional<double> maximum = std::nullopt,
        std::optional<double> step = std::nullopt);

    [[nodiscard]] const std::optional<double>& minimum() const noexcept;
    [[nodiscard]] const std::optional<double>& maximum() const noexcept;
    [[nodiscard]] const std::optional<double>& step() const noexcept;
    [[nodiscard]] bool accepts(double value) const noexcept;

private:
    std::optional<double> minimum_;
    std::optional<double> maximum_;
    std::optional<double> step_;
};

class EnumOption final {
public:
    EnumOption(EnumOptionKey key, std::string display_name, std::string description = {});

    [[nodiscard]] const EnumOptionKey& key() const noexcept;
    [[nodiscard]] const std::string& display_name() const noexcept;
    [[nodiscard]] const std::string& description() const noexcept;

private:
    EnumOptionKey key_;
    std::string display_name_;
    std::string description_;
};

class ParameterType final {
public:
    [[nodiscard]] static ParameterType boolean();
    [[nodiscard]] static ParameterType integer(NumericConstraint constraint = {});
    [[nodiscard]] static ParameterType floating_point(NumericConstraint constraint = {});
    [[nodiscard]] static ParameterType string();
    [[nodiscard]] static ParameterType enumeration(std::vector<EnumOption> options);
    [[nodiscard]] static ParameterType array(
        ParameterType element_type,
        std::size_t minimum_items = 0U,
        std::optional<std::size_t> maximum_items = std::nullopt);
    [[nodiscard]] static ParameterType geometry(GeometryKind geometry_kind);
    [[nodiscard]] static ParameterType region_of_interest(
        std::vector<GeometryKind> allowed_shapes);
    [[nodiscard]] static ParameterType resource_reference();
    [[nodiscard]] static ParameterType model_reference();
    [[nodiscard]] static ParameterType calibration_reference();

    [[nodiscard]] ParameterKind kind() const noexcept;
    [[nodiscard]] const std::optional<NumericConstraint>& numeric_constraint() const noexcept;
    [[nodiscard]] const std::vector<EnumOption>& enum_options() const noexcept;
    [[nodiscard]] const std::shared_ptr<const ParameterType>& element_type() const noexcept;
    [[nodiscard]] std::size_t minimum_items() const noexcept;
    [[nodiscard]] const std::optional<std::size_t>& maximum_items() const noexcept;
    [[nodiscard]] const std::optional<GeometryKind>& geometry_kind() const noexcept;
    [[nodiscard]] const std::vector<GeometryKind>& allowed_roi_shapes() const noexcept;
    [[nodiscard]] bool accepts(const ParameterValue& value) const noexcept;

private:
    explicit ParameterType(ParameterKind kind);

    ParameterKind kind_;
    std::optional<NumericConstraint> numeric_constraint_;
    std::vector<EnumOption> enum_options_;
    std::shared_ptr<const ParameterType> element_type_;
    std::size_t minimum_items_ = 0U;
    std::optional<std::size_t> maximum_items_;
    std::optional<GeometryKind> geometry_kind_;
    std::vector<GeometryKind> allowed_roi_shapes_;
};

enum class PropertyEditorControl : std::uint32_t {
    automatic = 0,
    checkbox = 1,
    numeric_input = 2,
    slider = 3,
    text_input = 4,
    multiline_text = 5,
    dropdown = 6,
    list = 7,
    geometry_canvas = 8,
    roi_canvas = 9,
    resource_picker = 10,
};

class PropertyEditorMetadata final {
public:
    PropertyEditorMetadata(
        std::vector<std::string> group_path = {},
        std::uint32_t order = 0U,
        PropertyEditorControl control = PropertyEditorControl::automatic,
        std::string unit = {});

    [[nodiscard]] const std::vector<std::string>& group_path() const noexcept;
    [[nodiscard]] std::uint32_t order() const noexcept;
    [[nodiscard]] PropertyEditorControl control() const noexcept;
    [[nodiscard]] const std::string& unit() const noexcept;

private:
    std::vector<std::string> group_path_;
    std::uint32_t order_;
    PropertyEditorControl control_;
    std::string unit_;
};

enum class VisibilityOperator : std::uint32_t {
    equals = 0,
    not_equals = 1,
};

class VisibilityCondition final {
public:
    VisibilityCondition(
        ParameterKey controller,
        VisibilityOperator operation,
        ParameterValue expected_value);

    [[nodiscard]] const ParameterKey& controller() const noexcept;
    [[nodiscard]] VisibilityOperator operation() const noexcept;
    [[nodiscard]] const ParameterValue& expected_value() const noexcept;

private:
    ParameterKey controller_;
    VisibilityOperator operation_;
    ParameterValue expected_value_;
};

class ParameterDefinition final {
public:
    ParameterDefinition(
        ParameterKey key,
        std::string display_name,
        ParameterType type,
        bool required = false,
        std::optional<ParameterValue> default_value = std::nullopt,
        std::string description = {},
        PropertyEditorMetadata editor = {},
        std::optional<VisibilityCondition> visibility = std::nullopt,
        bool recipe_overridable = false,
        bool runtime_bindable = false);

    [[nodiscard]] const ParameterKey& key() const noexcept;
    [[nodiscard]] const std::string& display_name() const noexcept;
    [[nodiscard]] const ParameterType& type() const noexcept;
    [[nodiscard]] bool required() const noexcept;
    [[nodiscard]] const std::optional<ParameterValue>& default_value() const noexcept;
    [[nodiscard]] const std::string& description() const noexcept;
    [[nodiscard]] const PropertyEditorMetadata& editor() const noexcept;
    [[nodiscard]] const std::optional<VisibilityCondition>& visibility() const noexcept;
    [[nodiscard]] bool recipe_overridable() const noexcept;
    [[nodiscard]] bool runtime_bindable() const noexcept;

private:
    ParameterKey key_;
    std::string display_name_;
    ParameterType type_;
    bool required_;
    std::optional<ParameterValue> default_value_;
    std::string description_;
    PropertyEditorMetadata editor_;
    std::optional<VisibilityCondition> visibility_;
    bool recipe_overridable_;
    bool runtime_bindable_;
};

}  // namespace nvision::core
