#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/tool_definition.hpp"

#include <cstddef>
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

using nvision::core::AxisAlignedRectangle;
using nvision::core::CoordinateSpaceId;
using nvision::workflow::EnumOption;
using nvision::core::EnumOptionKey;
using nvision::workflow::GeometryKind;
using nvision::core::IntegrityDigest;
using nvision::core::Line2D;
using nvision::workflow::NumericConstraint;
using nvision::workflow::ParameterDefinition;
using nvision::core::ParameterKey;
using nvision::workflow::ParameterKind;
using nvision::workflow::ParameterType;
using nvision::workflow::ParameterValue;
using nvision::core::Point2D;
using nvision::workflow::PropertyEditorControl;
using nvision::workflow::PropertyEditorMetadata;
using nvision::core::ResourceId;
using nvision::core::ResourceIntegrity;
using nvision::core::ResourceKind;
using nvision::core::ResourceReference;
using nvision::core::SemanticVersion;
using nvision::workflow::ToolCatalogMetadata;
using nvision::workflow::ToolTypeDefinition;
using nvision::core::ToolTypeId;
using nvision::workflow::VisibilityCondition;
using nvision::workflow::VisibilityOperator;

template <typename Action>
bool rejects_invalid_argument(Action action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

CoordinateSpaceId image_space() {
    return CoordinateSpaceId("nvision.frame.parameter-test.image-pixel");
}

ResourceIntegrity integrity() {
    return ResourceIntegrity(
        IntegrityDigest::parse(
            "sha256:3bec1d7190001d1dc1f4ef009013b1abafb1d6a7d725840f15c54920e639e313"),
        512U);
}

ResourceReference resource(const ResourceKind kind, std::string id) {
    return ResourceReference(
        ResourceId(std::move(id)),
        SemanticVersion(1U, 2U, 3U),
        kind,
        integrity());
}

ParameterType mode_type() {
    return ParameterType::enumeration(
        {EnumOption(EnumOptionKey("automatic"), "Automatic"),
         EnumOption(EnumOptionKey("manual"), "Manual", "Use the configured value.")});
}

ParameterDefinition mode_parameter() {
    return ParameterDefinition(
        ParameterKey("mode"),
        "Mode",
        mode_type(),
        true,
        ParameterValue::enumeration(EnumOptionKey("automatic")),
        "Select automatic or manual configuration.",
        PropertyEditorMetadata(
            {"Detection", "Threshold"},
            10U,
            PropertyEditorControl::dropdown));
}

ToolTypeDefinition tool_with_parameters(std::vector<ParameterDefinition> parameters) {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.processing.parameter-schema-test"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Schema Test", {"Image Processing"}),
        {},
        nvision::workflow::ToolOrigin::built_in(),
        {},
        std::move(parameters));
}

void schemas_describe_enum_array_geometry_and_roi_editors() {
    const ParameterDefinition mode = mode_parameter();
    NVISION_REQUIRE(mode.type().kind() == ParameterKind::enumeration);
    NVISION_REQUIRE(mode.type().enum_options().size() == 2U);
    NVISION_REQUIRE(mode.default_value()->enumeration_value().value() == "automatic");
    NVISION_REQUIRE(mode.editor().group_path().size() == 2U);
    NVISION_REQUIRE(mode.editor().order() == 10U);
    NVISION_REQUIRE(mode.editor().control() == PropertyEditorControl::dropdown);

    const ParameterDefinition weights(
        ParameterKey("weights"),
        "Weights",
        ParameterType::array(
            ParameterType::floating_point(NumericConstraint(0.0, 1.0, 0.05)),
            1U,
            4U),
        true,
        ParameterValue::array(
            {ParameterValue::floating_point(0.25), ParameterValue::floating_point(0.75)}),
        {},
        PropertyEditorMetadata({"Scoring"}, 20U, PropertyEditorControl::list));
    NVISION_REQUIRE(weights.type().element_type()->kind() == ParameterKind::floating_point);
    NVISION_REQUIRE(weights.type().minimum_items() == 1U);
    NVISION_REQUIRE(weights.type().maximum_items() == 4U);
    NVISION_REQUIRE(weights.default_value()->array_value().size() == 2U);

    const ParameterDefinition anchor(
        ParameterKey("anchor"),
        "Anchor",
        ParameterType::geometry(GeometryKind::point),
        false,
        ParameterValue::geometry(Point2D(image_space(), 10.0, 12.0)),
        {},
        PropertyEditorMetadata({"Geometry"}, 30U, PropertyEditorControl::geometry_canvas));
    NVISION_REQUIRE(anchor.default_value()->geometry_kind() == GeometryKind::point);

    const ParameterDefinition search_region(
        ParameterKey("search-region"),
        "Search Region",
        ParameterType::region_of_interest(
            {GeometryKind::rectangle, GeometryKind::polygon}),
        true,
        ParameterValue::region_of_interest(
            AxisAlignedRectangle(image_space(), 1.0, 2.0, 100.0, 80.0)),
        {},
        PropertyEditorMetadata({"Geometry"}, 40U, PropertyEditorControl::roi_canvas));
    NVISION_REQUIRE(search_region.type().allowed_roi_shapes().size() == 2U);
    NVISION_REQUIRE(search_region.default_value()->geometry_kind() == GeometryKind::rectangle);
}

void schemas_keep_versioned_resource_kinds_explicit() {
    const ParameterDefinition template_asset(
        ParameterKey("template"),
        "Template",
        ParameterType::resource_reference(),
        true,
        ParameterValue::resource_reference(
            resource(ResourceKind::template_asset, "resource.template.primary")),
        {},
        PropertyEditorMetadata({}, 0U, PropertyEditorControl::resource_picker));
    const ParameterDefinition model(
        ParameterKey("model"),
        "Model",
        ParameterType::model_reference(),
        true,
        ParameterValue::model_reference(resource(ResourceKind::model, "resource.model.primary")),
        {},
        PropertyEditorMetadata({}, 1U, PropertyEditorControl::resource_picker));
    const ParameterDefinition calibration(
        ParameterKey("calibration"),
        "Calibration",
        ParameterType::calibration_reference(),
        false,
        ParameterValue::calibration_reference(
            resource(ResourceKind::calibration, "resource.calibration.primary")),
        {},
        PropertyEditorMetadata({}, 2U, PropertyEditorControl::resource_picker));

    NVISION_REQUIRE(template_asset.default_value()->reference_value().kind() ==
                    ResourceKind::template_asset);
    NVISION_REQUIRE(model.default_value()->reference_value().version().to_string() == "1.2.3");
    NVISION_REQUIRE(calibration.default_value()->reference_value().integrity() == integrity());
}

void tool_definitions_validate_declarative_visibility_conditions() {
    const ParameterDefinition manual_value(
        ParameterKey("manual-value"),
        "Manual Value",
        ParameterType::floating_point(NumericConstraint(0.0, 255.0, 1.0)),
        false,
        ParameterValue::floating_point(128.0),
        {},
        PropertyEditorMetadata(
            {"Detection", "Threshold"},
            20U,
            PropertyEditorControl::slider,
            "level"),
        VisibilityCondition(
            ParameterKey("mode"),
            VisibilityOperator::equals,
            ParameterValue::enumeration(EnumOptionKey("manual"))),
        true,
        true);
    const ToolTypeDefinition tool =
        tool_with_parameters({mode_parameter(), manual_value});

    NVISION_REQUIRE(tool.parameters().size() == 2U);
    const auto* resolved = tool.find_parameter(ParameterKey("manual-value"));
    NVISION_REQUIRE(resolved != nullptr);
    NVISION_REQUIRE(resolved->visibility()->controller().value() == "mode");
    NVISION_REQUIRE(resolved->visibility()->operation() == VisibilityOperator::equals);
    NVISION_REQUIRE(resolved->visibility()->expected_value().enumeration_value().value() ==
                    "manual");
    NVISION_REQUIRE(resolved->recipe_overridable());
    NVISION_REQUIRE(resolved->runtime_bindable());
    NVISION_REQUIRE(resolved->editor().unit() == "level");
}

void type_constraints_reject_incompatible_defaults() {
    NVISION_REQUIRE(rejects_invalid_argument([] {
        NumericConstraint invalid(2.0, 1.0);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        NumericConstraint invalid(0.0, 1.0, 0.0);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        NumericConstraint invalid(std::numeric_limits<double>::infinity());
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterValue::floating_point(
            std::numeric_limits<double>::quiet_NaN());
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterType::enumeration({});
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterType::enumeration(
            {EnumOption(EnumOptionKey("same"), "One"),
             EnumOption(EnumOptionKey("same"), "Two")});
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterType::array(ParameterType::integer(), 3U, 2U);
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterType::geometry(static_cast<GeometryKind>(999U));
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterType::region_of_interest({GeometryKind::point});
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterValue::region_of_interest(
            Line2D(Point2D(image_space(), 0.0, 0.0), Point2D(image_space(), 1.0, 1.0)));
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = ParameterValue::model_reference(
            resource(ResourceKind::calibration, "resource.calibration.wrong"));
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ParameterDefinition invalid(
            ParameterKey("threshold"),
            "Threshold",
            ParameterType::floating_point(NumericConstraint(0.0, 1.0)),
            false,
            ParameterValue::floating_point(2.0));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ParameterDefinition invalid(
            ParameterKey("mode"),
            "Mode",
            mode_type(),
            false,
            ParameterValue::enumeration(EnumOptionKey("missing")));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ParameterDefinition invalid(
            ParameterKey("label"),
            "Label",
            ParameterType::string(),
            false,
            std::nullopt,
            {},
            PropertyEditorMetadata({}, 0U, PropertyEditorControl::slider));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ParameterDefinition invalid(
            ParameterKey("label"),
            "Label",
            ParameterType::string(),
            false,
            std::nullopt,
            {},
            PropertyEditorMetadata({}, 0U, PropertyEditorControl::text_input, "mm"));
    }));
}

// Assignability is about the whole accepted domain, not the kind and one default. Every
// case here would bind cleanly under a kind comparison and then fail at execution.
void parameter_domains_are_assignable_only_when_they_are_contained() {
    const auto option = [](const char* key) {
        return EnumOption(EnumOptionKey(key), key);
    };
    const ParameterType wide_enum =
        ParameterType::enumeration({option("fast"), option("accurate")});
    const ParameterType narrow_enum = ParameterType::enumeration({option("fast")});
    NVISION_REQUIRE(narrow_enum.is_assignable_to(wide_enum));
    NVISION_REQUIRE(!wide_enum.is_assignable_to(narrow_enum));

    const ParameterType wide_range =
        ParameterType::floating_point(NumericConstraint(0.0, 10.0));
    const ParameterType narrow_range =
        ParameterType::floating_point(NumericConstraint(2.0, 5.0));
    const ParameterType unbounded = ParameterType::floating_point();
    NVISION_REQUIRE(narrow_range.is_assignable_to(wide_range));
    NVISION_REQUIRE(!wide_range.is_assignable_to(narrow_range));
    NVISION_REQUIRE(wide_range.is_assignable_to(unbounded));
    // An unbounded exposed parameter can offer a value no bounded internal one accepts.
    NVISION_REQUIRE(!unbounded.is_assignable_to(wide_range));

    // A grid the target does not share cannot be shown to land on it.
    const ParameterType stepped =
        ParameterType::integer(NumericConstraint(0.0, 10.0, 2.0));
    const ParameterType unstepped = ParameterType::integer(NumericConstraint(0.0, 10.0));
    NVISION_REQUIRE(unstepped.is_assignable_to(unstepped));
    NVISION_REQUIRE(stepped.is_assignable_to(unstepped));
    NVISION_REQUIRE(!unstepped.is_assignable_to(stepped));

    // A different kind is never assignable, whatever the domains look like.
    NVISION_REQUIRE(!ParameterType::string().is_assignable_to(wide_enum));
    NVISION_REQUIRE(!ParameterType::integer().is_assignable_to(ParameterType::floating_point()));

    const ParameterType narrow_array =
        ParameterType::array(narrow_enum, 1U, std::optional<std::size_t>(3U));
    const ParameterType wide_array =
        ParameterType::array(wide_enum, 0U, std::optional<std::size_t>(8U));
    NVISION_REQUIRE(narrow_array.is_assignable_to(wide_array));
    NVISION_REQUIRE(!wide_array.is_assignable_to(narrow_array));
    // An element type that does not fit sinks the array even when the counts do.
    const ParameterType wide_elements_narrow_counts =
        ParameterType::array(wide_enum, 1U, std::optional<std::size_t>(3U));
    const ParameterType narrow_elements_wide_counts =
        ParameterType::array(narrow_enum, 0U, std::optional<std::size_t>(8U));
    NVISION_REQUIRE(
        !wide_elements_narrow_counts.is_assignable_to(narrow_elements_wide_counts));
    // An unbounded count cannot be shown to fit inside a bounded one.
    NVISION_REQUIRE(!ParameterType::array(narrow_enum, 1U).is_assignable_to(wide_array));

    NVISION_REQUIRE(ParameterType::geometry(GeometryKind::circle)
                        .is_assignable_to(ParameterType::geometry(GeometryKind::circle)));
    NVISION_REQUIRE(!ParameterType::geometry(GeometryKind::circle)
                         .is_assignable_to(ParameterType::geometry(GeometryKind::rectangle)));

    const ParameterType rect_only =
        ParameterType::region_of_interest({GeometryKind::rectangle});
    const ParameterType rect_or_polygon =
        ParameterType::region_of_interest({GeometryKind::rectangle, GeometryKind::polygon});
    NVISION_REQUIRE(rect_only.is_assignable_to(rect_or_polygon));
    NVISION_REQUIRE(!rect_or_polygon.is_assignable_to(rect_only));

    NVISION_REQUIRE(
        ParameterType::model_reference().is_assignable_to(ParameterType::model_reference()));
    NVISION_REQUIRE(
        !ParameterType::model_reference().is_assignable_to(ParameterType::resource_reference()));
}

void tool_definitions_reject_invalid_condition_graphs() {
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = tool_with_parameters({mode_parameter(), mode_parameter()});
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = tool_with_parameters(
            {ParameterDefinition(
                ParameterKey("value"),
                "Value",
                ParameterType::integer(),
                false,
                std::nullopt,
                {},
                {},
                VisibilityCondition(
                    ParameterKey("missing"),
                    VisibilityOperator::equals,
                    ParameterValue::boolean(true))) });
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = tool_with_parameters(
            {mode_parameter(),
             ParameterDefinition(
                 ParameterKey("value"),
                 "Value",
                 ParameterType::integer(),
                 false,
                 std::nullopt,
                 {},
                 {},
                 VisibilityCondition(
                     ParameterKey("mode"),
                     VisibilityOperator::equals,
                     ParameterValue::boolean(true))) });
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = tool_with_parameters(
            {ParameterDefinition(
                ParameterKey("enabled"),
                "Enabled",
                ParameterType::boolean(),
                false,
                ParameterValue::boolean(true),
                {},
                {},
                VisibilityCondition(
                    ParameterKey("enabled"),
                    VisibilityOperator::equals,
                    ParameterValue::boolean(true))) });
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto invalid = tool_with_parameters(
            {ParameterDefinition(
                 ParameterKey("first"),
                 "First",
                 ParameterType::boolean(),
                 false,
                 ParameterValue::boolean(true),
                 {},
                 {},
                 VisibilityCondition(
                     ParameterKey("second"),
                     VisibilityOperator::equals,
                     ParameterValue::boolean(true))),
             ParameterDefinition(
                 ParameterKey("second"),
                 "Second",
                 ParameterType::boolean(),
                 false,
                 ParameterValue::boolean(true),
                 {},
                 {},
                 VisibilityCondition(
                     ParameterKey("first"),
                     VisibilityOperator::equals,
                     ParameterValue::boolean(true))) });
        static_cast<void>(invalid);
    }));
}

}  // namespace

int main() {
    schemas_describe_enum_array_geometry_and_roi_editors();
    schemas_keep_versioned_resource_kinds_explicit();
    tool_definitions_validate_declarative_visibility_conditions();
    type_constraints_reject_incompatible_defaults();
    parameter_domains_are_assignable_only_when_they_are_contained();
    tool_definitions_reject_invalid_condition_graphs();
    return EXIT_SUCCESS;
}
