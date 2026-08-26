#include "nvision/core/subworkflow.hpp"
#include "nvision/core/workflow_node.hpp"

#include <cstdlib>
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
using nvision::core::DiagnosticSeverity;
using nvision::core::GeometryKind;
using nvision::core::NodeConfiguration;
using nvision::core::NodeParameterAssignment;
using nvision::core::NumericConstraint;
using nvision::core::ParameterDefinition;
using nvision::core::ParameterKey;
using nvision::core::ParameterType;
using nvision::core::ParameterValue;
using nvision::core::PortDataType;
using nvision::core::PortDirection;
using nvision::core::PortKey;
using nvision::core::SemanticVersion;
using nvision::core::SubworkflowBinding;
using nvision::core::SubworkflowDefinition;
using nvision::core::SubworkflowParameterPromotion;
using nvision::core::SubworkflowPortMapping;
using nvision::core::ToolCatalogMetadata;
using nvision::core::ToolPortDefinition;
using nvision::core::ToolRegistry;
using nvision::core::ToolResolutionStatus;
using nvision::core::ToolTypeDefinition;
using nvision::core::ToolTypeId;
using nvision::core::WorkflowDiagnostic;
using nvision::core::WorkflowNode;
using nvision::core::WorkflowNodeBinding;
using nvision::core::WorkflowNodeId;

template <typename Action>
bool rejects_invalid_argument(Action action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

ToolPortDefinition port(
    std::string key,
    const PortDirection direction,
    const PortDataType type) {
    return ToolPortDefinition(
        PortKey(std::move(key)),
        direction == PortDirection::input ? "Input" : "Output",
        direction,
        type,
        true);
}

AxisAlignedRectangle sample_rectangle() {
    return AxisAlignedRectangle(CoordinateSpaceId("image.pixel"), 4.0, 8.0, 64.0, 32.0);
}

ParameterDefinition roi_parameter() {
    return ParameterDefinition(
        ParameterKey("roi"),
        "Region Of Interest",
        ParameterType::region_of_interest({GeometryKind::rectangle}),
        true,
        ParameterValue::region_of_interest(sample_rectangle()));
}

ParameterDefinition confidence_parameter() {
    return ParameterDefinition(
        ParameterKey("minimum-confidence"),
        "Minimum Confidence",
        ParameterType::floating_point(NumericConstraint(0.0, 1.0)),
        true,
        ParameterValue::floating_point(0.5));
}

// Generic crop tool: image view in, image view out, ROI parameter with a default.
ToolTypeDefinition crop_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.processing.crop"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Crop", {"Image Processing"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("image", PortDirection::output, PortDataType::image_view)},
        nvision::core::ToolOrigin::built_in(),
        {},
        {roi_parameter()});
}

// Generic recognition tool: image view in, text result out.
ToolTypeDefinition text_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.recognition.text-recognition"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Text Recognition", {"Recognition", "Text"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("text", PortDirection::output, PortDataType::text_result)},
        nvision::core::ToolOrigin::built_in(),
        {},
        {confidence_parameter()});
}

// Declares a required threshold without a default so that an unset value is an error.
ToolTypeDefinition compare_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.logic.compare"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Compare", {"Logic and Results"}),
        {port("value", PortDirection::input, PortDataType::floating_point),
         port("decision", PortDirection::output, PortDataType::decision)},
        nvision::core::ToolOrigin::built_in(),
        {},
        {ParameterDefinition(
            ParameterKey("threshold"),
            "Threshold",
            ParameterType::floating_point(NumericConstraint(0.0, 1.0)),
            true)});
}

WorkflowNode crop_node() {
    return WorkflowNode(
        WorkflowNodeId("node.crop"),
        ToolTypeId("nvision.tool.processing.crop"),
        SemanticVersion(1U, 0U, 0U),
        "Crop",
        NodeConfiguration({NodeParameterAssignment(
            ParameterKey("roi"), ParameterValue::region_of_interest(sample_rectangle()))}));
}

WorkflowNode text_node() {
    return WorkflowNode(
        WorkflowNodeId("node.text"),
        ToolTypeId("nvision.tool.recognition.text-recognition"),
        SemanticVersion(1U, 0U, 0U),
        "Text Recognition",
        NodeConfiguration({NodeParameterAssignment(
            ParameterKey("minimum-confidence"), ParameterValue::floating_point(0.8))}));
}

const WorkflowDiagnostic* find_diagnostic(
    const std::vector<WorkflowDiagnostic>& diagnostics,
    const std::string& code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code().value() == code) {
            return &diagnostic;
        }
    }
    return nullptr;
}

ToolRegistry reference_registry() {
    ToolRegistry registry;
    static_cast<void>(registry.register_definition(crop_definition()));
    static_cast<void>(registry.register_definition(text_definition()));
    static_cast<void>(registry.register_definition(compare_definition()));
    return registry;
}

void unresolved_nodes_keep_identity_and_configuration() {
    const ToolRegistry registry = reference_registry();

    const WorkflowNode missing_plugin(
        WorkflowNodeId("node.custom"),
        ToolTypeId("nvision.plugin.tool.custom-locator"),
        SemanticVersion(2U, 1U, 0U),
        "Custom Locator",
        NodeConfiguration(
            {NodeParameterAssignment(
                 ParameterKey("search-angle"), ParameterValue::floating_point(12.5)),
             NodeParameterAssignment(
                 ParameterKey("model"), ParameterValue::string("golden-template"))}));

    const WorkflowNodeBinding unknown = bind_workflow_node(missing_plugin, registry);
    NVISION_REQUIRE(!unknown.is_resolved());
    NVISION_REQUIRE(unknown.status() == ToolResolutionStatus::unknown_tool);
    NVISION_REQUIRE(!unknown.definition());
    NVISION_REQUIRE(unknown.has_errors());
    NVISION_REQUIRE(unknown.diagnostics().size() == 1U);
    NVISION_REQUIRE(unknown.diagnostics()[0].code().value() == "UNKNOWN_TOOL_TYPE");
    NVISION_REQUIRE(unknown.diagnostics()[0].severity() == DiagnosticSeverity::error);
    NVISION_REQUIRE(unknown.diagnostics()[0].node_id()->value() == "node.custom");

    // The authored identity and every parameter survive the missing plugin.
    NVISION_REQUIRE(unknown.node().tool_type_id().value() == "nvision.plugin.tool.custom-locator");
    NVISION_REQUIRE(unknown.node().tool_version().to_string() == "2.1.0");
    NVISION_REQUIRE(unknown.node().configuration().size() == 2U);
    NVISION_REQUIRE(
        unknown.node().configuration().assignments()[0].key().value() == "search-angle");
    NVISION_REQUIRE(
        unknown.node().configuration().find(ParameterKey("model"))->string_value() ==
        "golden-template");

    const WorkflowNode future_version(
        WorkflowNodeId("node.crop"),
        ToolTypeId("nvision.tool.processing.crop"),
        SemanticVersion(3U, 0U, 0U),
        "Crop",
        NodeConfiguration({NodeParameterAssignment(
            ParameterKey("roi"), ParameterValue::region_of_interest(sample_rectangle()))}));

    const WorkflowNodeBinding unsupported = bind_workflow_node(future_version, registry);
    NVISION_REQUIRE(unsupported.status() == ToolResolutionStatus::unsupported_version);
    NVISION_REQUIRE(!unsupported.definition());
    NVISION_REQUIRE(unsupported.diagnostics().size() == 1U);
    NVISION_REQUIRE(unsupported.diagnostics()[0].code().value() == "UNSUPPORTED_TOOL_VERSION");
    NVISION_REQUIRE(unsupported.node().configuration().size() == 1U);
    NVISION_REQUIRE(unsupported.node().tool_version().to_string() == "3.0.0");
}

void resolved_nodes_report_schema_problems_without_dropping_values() {
    const ToolRegistry registry = reference_registry();

    const WorkflowNodeBinding valid = bind_workflow_node(crop_node(), registry);
    NVISION_REQUIRE(valid.is_resolved());
    NVISION_REQUIRE(valid.definition()->id().value() == "nvision.tool.processing.crop");
    NVISION_REQUIRE(valid.diagnostics().empty());
    NVISION_REQUIRE(!valid.has_errors());

    const WorkflowNode drifted(
        WorkflowNodeId("node.text"),
        ToolTypeId("nvision.tool.recognition.text-recognition"),
        SemanticVersion(1U, 0U, 0U),
        "Text Recognition",
        NodeConfiguration(
            {NodeParameterAssignment(
                 ParameterKey("minimum-confidence"), ParameterValue::floating_point(4.0)),
             NodeParameterAssignment(
                 ParameterKey("orientation-steps"), ParameterValue::integer(2))}));

    const WorkflowNodeBinding binding = bind_workflow_node(drifted, registry);
    NVISION_REQUIRE(binding.is_resolved());
    NVISION_REQUIRE(binding.has_errors());
    NVISION_REQUIRE(binding.diagnostics().size() == 2U);

    const auto* mismatch = find_diagnostic(binding.diagnostics(), "PARAMETER_TYPE_MISMATCH");
    NVISION_REQUIRE(mismatch != nullptr);
    NVISION_REQUIRE(mismatch->severity() == DiagnosticSeverity::error);
    NVISION_REQUIRE(mismatch->parameter_key()->value() == "minimum-confidence");

    // An undeclared key is reported, never removed, so a downgrade stays reversible.
    const auto* unknown = find_diagnostic(binding.diagnostics(), "UNKNOWN_PARAMETER");
    NVISION_REQUIRE(unknown != nullptr);
    NVISION_REQUIRE(unknown->severity() == DiagnosticSeverity::warning);
    NVISION_REQUIRE(unknown->parameter_key()->value() == "orientation-steps");
    NVISION_REQUIRE(binding.node().configuration().size() == 2U);
    NVISION_REQUIRE(
        binding.node().configuration().find(ParameterKey("orientation-steps"))->integer_value() ==
        2);

    const WorkflowNode incomplete(
        WorkflowNodeId("node.compare"),
        ToolTypeId("nvision.tool.logic.compare"),
        SemanticVersion(1U, 0U, 0U),
        "Compare");

    const WorkflowNodeBinding missing = bind_workflow_node(incomplete, registry);
    NVISION_REQUIRE(missing.is_resolved());
    NVISION_REQUIRE(missing.diagnostics().size() == 1U);
    NVISION_REQUIRE(missing.diagnostics()[0].code().value() == "MISSING_REQUIRED_PARAMETER");
    NVISION_REQUIRE(missing.diagnostics()[0].parameter_key()->value() == "threshold");

    // A required parameter that declares a default is already satisfied.
    const WorkflowNode defaulted(
        WorkflowNodeId("node.crop-default"),
        ToolTypeId("nvision.tool.processing.crop"),
        SemanticVersion(1U, 0U, 0U),
        "Crop");
    NVISION_REQUIRE(bind_workflow_node(defaulted, registry).diagnostics().empty());
}

void invalid_node_contracts_are_rejected() {
    NVISION_REQUIRE(rejects_invalid_argument([] {
        NodeConfiguration invalid(
            {NodeParameterAssignment(ParameterKey("roi"), ParameterValue::integer(1)),
             NodeParameterAssignment(ParameterKey("roi"), ParameterValue::integer(2))});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowNode invalid(
            WorkflowNodeId("node.blank"),
            ToolTypeId("nvision.tool.processing.crop"),
            SemanticVersion(1U, 0U, 0U),
            "   ");
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowDiagnostic invalid = WorkflowDiagnostic::node(
            nvision::core::ResultCode("UNKNOWN_TOOL_TYPE"),
            static_cast<DiagnosticSeverity>(999U),
            "unsupported severity",
            WorkflowNodeId("node.crop"));
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolRegistry registry;
        const auto definition = registry.register_definition(crop_definition());
        WorkflowNodeBinding invalid(text_node(), ToolResolutionStatus::resolved, definition, {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowNodeBinding invalid(crop_node(), ToolResolutionStatus::resolved, nullptr, {});
    }));
}

SubworkflowDefinition text_region_subworkflow() {
    // A reusable "crop then recognize" pipeline published as an ordinary tool contract.
    return SubworkflowDefinition(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.region-text"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Region Text", {"Recognition", "Text"}),
            {port("image", PortDirection::input, PortDataType::image_view),
             port("text", PortDirection::output, PortDataType::text_result)},
            nvision::core::ToolOrigin::built_in(),
            {},
            {roi_parameter(), confidence_parameter()}),
        {crop_node(), text_node()},
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))},
        {SubworkflowParameterPromotion(
             ParameterKey("roi"), WorkflowNodeId("node.crop"), ParameterKey("roi")),
         SubworkflowParameterPromotion(
             ParameterKey("minimum-confidence"),
             WorkflowNodeId("node.text"),
             ParameterKey("minimum-confidence"))});
}

void subworkflows_expose_a_reusable_graph_through_a_tool_interface() {
    const SubworkflowDefinition subworkflow = text_region_subworkflow();
    const ToolRegistry registry = reference_registry();

    NVISION_REQUIRE(subworkflow.interface_definition().catalog().category_path()[0] ==
                    "Recognition");
    NVISION_REQUIRE(subworkflow.nodes().size() == 2U);
    NVISION_REQUIRE(subworkflow.find_node(WorkflowNodeId("node.text")) != nullptr);
    NVISION_REQUIRE(subworkflow.find_node(WorkflowNodeId("node.absent")) == nullptr);

    const SubworkflowBinding binding = bind_subworkflow(subworkflow, registry);
    NVISION_REQUIRE(binding.is_fully_resolved());
    NVISION_REQUIRE(!binding.has_errors());
    NVISION_REQUIRE(binding.diagnostics().empty());
    NVISION_REQUIRE(binding.nodes().size() == 2U);
    NVISION_REQUIRE(binding.find_node(WorkflowNodeId("node.crop"))->is_resolved());
    NVISION_REQUIRE(binding.find_node(WorkflowNodeId("node.crop"))
                        ->node()
                        .configuration()
                        .contains(ParameterKey("roi")));

    // One exposed input may feed several internal nodes.
    const SubworkflowDefinition fan_out(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.dual-region-text"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Dual Region Text", {"Recognition", "Text"}),
            {port("image", PortDirection::input, PortDataType::image_view),
             port("text", PortDirection::output, PortDataType::text_result)}),
        {crop_node(), text_node()},
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.text"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});
    NVISION_REQUIRE(!bind_subworkflow(fan_out, registry).has_errors());
}

void invalid_subworkflow_structures_are_rejected() {
    const ToolTypeDefinition interface_definition(
        ToolTypeId("nvision.subworkflow.recognition.region-text"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Region Text", {"Recognition", "Text"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("text", PortDirection::output, PortDataType::text_result)});

    const SubworkflowPortMapping image_mapping(
        PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"), PortKey("image"));
    const SubworkflowPortMapping text_mapping(
        PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"), PortKey("text"));

    NVISION_REQUIRE(rejects_invalid_argument([&interface_definition] {
        SubworkflowDefinition invalid(interface_definition, {}, {});
    }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                {crop_node(), crop_node()},
                {image_mapping, text_mapping});
        }));
    NVISION_REQUIRE(rejects_invalid_argument([&interface_definition, &image_mapping] {
        // A subworkflow that contains itself can never be expanded.
        SubworkflowDefinition invalid(
            interface_definition,
            {crop_node(),
             WorkflowNode(
                 WorkflowNodeId("node.self"),
                 ToolTypeId("nvision.subworkflow.recognition.region-text"),
                 SemanticVersion(1U, 0U, 0U),
                 "Self")},
            {image_mapping,
             SubworkflowPortMapping(
                 PortKey("text"), PortDirection::output, WorkflowNodeId("node.self"),
                 PortKey("text"))});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&interface_definition, &image_mapping] {
        // The declared output port is never produced.
        SubworkflowDefinition invalid(
            interface_definition, {crop_node(), text_node()}, {image_mapping});
    }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                {crop_node(), text_node()},
                {image_mapping,
                 text_mapping,
                 SubworkflowPortMapping(
                     PortKey("text"), PortDirection::output, WorkflowNodeId("node.crop"),
                     PortKey("image"))});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                {crop_node(), text_node()},
                {image_mapping, image_mapping, text_mapping});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                {crop_node(), text_node()},
                {image_mapping,
                 text_mapping,
                 SubworkflowPortMapping(
                     PortKey("mask"), PortDirection::input, WorkflowNodeId("node.crop"),
                     PortKey("mask"))});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                {crop_node(), text_node()},
                {image_mapping,
                 text_mapping,
                 SubworkflowPortMapping(
                     PortKey("image"), PortDirection::input, WorkflowNodeId("node.absent"),
                     PortKey("image"))});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                {crop_node(), text_node()},
                {image_mapping, text_mapping},
                {SubworkflowParameterPromotion(
                    ParameterKey("roi"), WorkflowNodeId("node.crop"), ParameterKey("roi"))});
        }));

    const ToolTypeDefinition with_roi(
        ToolTypeId("nvision.subworkflow.recognition.region-text"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Region Text", {"Recognition", "Text"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("text", PortDirection::output, PortDataType::text_result)},
        nvision::core::ToolOrigin::built_in(),
        {},
        {roi_parameter()});

    NVISION_REQUIRE(rejects_invalid_argument([&with_roi, &image_mapping, &text_mapping] {
        SubworkflowDefinition invalid(
            with_roi,
            {crop_node(), text_node()},
            {image_mapping, text_mapping},
            {SubworkflowParameterPromotion(
                 ParameterKey("roi"), WorkflowNodeId("node.crop"), ParameterKey("roi")),
             SubworkflowParameterPromotion(
                 ParameterKey("roi"), WorkflowNodeId("node.text"), ParameterKey("roi"))});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&with_roi, &image_mapping, &text_mapping] {
        SubworkflowDefinition invalid(
            with_roi,
            {crop_node(), text_node()},
            {image_mapping, text_mapping},
            {SubworkflowParameterPromotion(
                ParameterKey("roi"), WorkflowNodeId("node.absent"), ParameterKey("roi"))});
    }));
}

void subworkflow_binding_reports_contract_mismatches() {
    const ToolRegistry registry = reference_registry();

    // The interface promises a decision, but the mapped node port produces text.
    const SubworkflowDefinition wrong_type(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.wrong-type"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Wrong Type", {"Recognition"}),
            {port("image", PortDirection::input, PortDataType::image_view),
             port("text", PortDirection::output, PortDataType::decision)}),
        {crop_node(), text_node()},
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});

    const SubworkflowBinding type_binding = bind_subworkflow(wrong_type, registry);
    NVISION_REQUIRE(type_binding.is_fully_resolved());
    NVISION_REQUIRE(type_binding.has_errors());
    NVISION_REQUIRE(find_diagnostic(type_binding.diagnostics(), "PORT_TYPE_MISMATCH") != nullptr);

    // The mapped node port does not exist in the bound tool version.
    const SubworkflowDefinition missing_port(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.missing-port"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Missing Port", {"Recognition"}),
            {port("mask", PortDirection::input, PortDataType::mask),
             port("text", PortDirection::output, PortDataType::text_result)}),
        {crop_node(), text_node()},
        {SubworkflowPortMapping(
             PortKey("mask"), PortDirection::input, WorkflowNodeId("node.crop"), PortKey("mask")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});

    const SubworkflowBinding port_binding = bind_subworkflow(missing_port, registry);
    NVISION_REQUIRE(port_binding.has_errors());
    const auto* missing = find_diagnostic(port_binding.diagnostics(), "MISSING_MAPPED_PORT");
    NVISION_REQUIRE(missing != nullptr);
    NVISION_REQUIRE(missing->port_key()->value() == "mask");
    NVISION_REQUIRE(missing->node_id()->value() == "node.crop");

    // Promotions must target a declared, type-compatible node parameter.
    const SubworkflowDefinition wrong_promotion(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.wrong-promotion"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Wrong Promotion", {"Recognition"}),
            {port("image", PortDirection::input, PortDataType::image_view),
             port("text", PortDirection::output, PortDataType::text_result)},
            nvision::core::ToolOrigin::built_in(),
            {},
            {roi_parameter(),
             ParameterDefinition(
                 ParameterKey("orientation-steps"),
                 "Orientation Steps",
                 ParameterType::integer(NumericConstraint(1.0, 4.0)),
                 true,
                 ParameterValue::integer(2))}),
        {crop_node(), text_node()},
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))},
        {SubworkflowParameterPromotion(
             ParameterKey("roi"),
             WorkflowNodeId("node.text"),
             ParameterKey("minimum-confidence")),
         SubworkflowParameterPromotion(
             ParameterKey("orientation-steps"),
             WorkflowNodeId("node.text"),
             ParameterKey("orientation-steps"))});

    const SubworkflowBinding promotion_binding = bind_subworkflow(wrong_promotion, registry);
    NVISION_REQUIRE(promotion_binding.has_errors());
    NVISION_REQUIRE(
        find_diagnostic(promotion_binding.diagnostics(), "PROMOTED_PARAMETER_TYPE_MISMATCH") !=
        nullptr);
    NVISION_REQUIRE(
        find_diagnostic(promotion_binding.diagnostics(), "MISSING_PROMOTED_PARAMETER") != nullptr);

    // An exposed parameter that reaches no node is reported without failing the bind.
    const SubworkflowDefinition unpromoted(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.unpromoted"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Unpromoted", {"Recognition"}),
            {port("image", PortDirection::input, PortDataType::image_view),
             port("text", PortDirection::output, PortDataType::text_result)},
            nvision::core::ToolOrigin::built_in(),
            {},
            {roi_parameter()}),
        {crop_node(), text_node()},
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});

    const SubworkflowBinding unpromoted_binding = bind_subworkflow(unpromoted, registry);
    NVISION_REQUIRE(!unpromoted_binding.has_errors());
    const auto* warning = find_diagnostic(unpromoted_binding.diagnostics(), "UNPROMOTED_PARAMETER");
    NVISION_REQUIRE(warning != nullptr);
    NVISION_REQUIRE(warning->severity() == DiagnosticSeverity::warning);
    NVISION_REQUIRE(!warning->node_id().has_value());
}

void unresolved_nodes_do_not_hide_behind_mapping_diagnostics() {
    ToolRegistry registry;
    static_cast<void>(registry.register_definition(text_definition()));

    const SubworkflowDefinition subworkflow = text_region_subworkflow();
    const SubworkflowBinding binding = bind_subworkflow(subworkflow, registry);

    NVISION_REQUIRE(!binding.is_fully_resolved());
    NVISION_REQUIRE(binding.has_errors());
    NVISION_REQUIRE(!binding.find_node(WorkflowNodeId("node.crop"))->is_resolved());
    NVISION_REQUIRE(binding.find_node(WorkflowNodeId("node.text"))->is_resolved());

    // Only the missing tool is reported; its mapping and promotion stay untouched.
    NVISION_REQUIRE(binding.diagnostics().empty());
    NVISION_REQUIRE(
        binding.find_node(WorkflowNodeId("node.crop"))->diagnostics()[0].code().value() ==
        "UNKNOWN_TOOL_TYPE");
    NVISION_REQUIRE(binding.find_node(WorkflowNodeId("node.crop"))
                        ->node()
                        .configuration()
                        .contains(ParameterKey("roi")));
    NVISION_REQUIRE(subworkflow.port_mappings().size() == 2U);
    NVISION_REQUIRE(subworkflow.parameter_promotions().size() == 2U);
}

}  // namespace

int main() {
    unresolved_nodes_keep_identity_and_configuration();
    resolved_nodes_report_schema_problems_without_dropping_values();
    invalid_node_contracts_are_rejected();
    subworkflows_expose_a_reusable_graph_through_a_tool_interface();
    invalid_subworkflow_structures_are_rejected();
    subworkflow_binding_reports_contract_mismatches();
    unresolved_nodes_do_not_hide_behind_mapping_diagnostics();
    return EXIT_SUCCESS;
}
