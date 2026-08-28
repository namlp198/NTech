#include "nvision/workflow/subworkflow.hpp"
#include "nvision/workflow/workflow_graph.hpp"
#include "nvision/workflow/workflow_node.hpp"

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
using nvision::workflow::DiagnosticSeverity;
using nvision::workflow::GeometryKind;
using nvision::workflow::NodeConfiguration;
using nvision::workflow::NodeParameterAssignment;
using nvision::workflow::NodePortAddress;
using nvision::workflow::NumericConstraint;
using nvision::workflow::ParameterDefinition;
using nvision::core::ParameterKey;
using nvision::workflow::ParameterType;
using nvision::workflow::ParameterValue;
using nvision::core::PortDataType;
using nvision::workflow::PortDirection;
using nvision::core::PortKey;
using nvision::core::SemanticVersion;
using nvision::workflow::SubworkflowDefinition;
using nvision::workflow::SubworkflowParameterPromotion;
using nvision::workflow::SubworkflowPortMapping;
using nvision::workflow::ToolCatalogMetadata;
using nvision::workflow::ToolPortDefinition;
using nvision::workflow::ToolRegistry;
using nvision::workflow::ToolResolutionStatus;
using nvision::workflow::ToolTypeDefinition;
using nvision::core::ToolTypeId;
using nvision::workflow::WorkflowConnection;
using nvision::workflow::WorkflowDiagnostic;
using nvision::workflow::WorkflowGraph;
using nvision::workflow::WorkflowGraphBinding;
using nvision::workflow::WorkflowNode;
using nvision::workflow::WorkflowNodeBinding;
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
    const PortDataType type,
    const bool required = true,
    const bool allows_multiple = false) {
    return ToolPortDefinition(
        PortKey(std::move(key)),
        direction == PortDirection::input ? "Input" : "Output",
        direction,
        type,
        required,
        allows_multiple);
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

// A source has no inputs, so a graph that starts here needs nothing from a caller.
ToolTypeDefinition source_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.acquisition.image-source"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Image Source", {"Acquisition"}),
        {port("image", PortDirection::output, PortDataType::image_view)});
}

ToolTypeDefinition crop_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.processing.crop"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Crop", {"Image Processing"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("image", PortDirection::output, PortDataType::image_view)},
        nvision::workflow::ToolOrigin::built_in(),
        {},
        {roi_parameter()});
}

ToolTypeDefinition text_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.recognition.text-recognition"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Text Recognition", {"Recognition", "Text"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("text", PortDirection::output, PortDataType::text_result)},
        nvision::workflow::ToolOrigin::built_in(),
        {},
        {confidence_parameter()});
}

ToolTypeDefinition sharpness_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.measurement.sharpness"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Sharpness", {"Measurement"}),
        {port("image", PortDirection::input, PortDataType::image_view),
         port("value", PortDirection::output, PortDataType::floating_point)});
}

// Declares a required threshold without a default so that an unset value is an error.
ToolTypeDefinition compare_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.logic.compare"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Compare", {"Logic and Results"}),
        {port("value", PortDirection::input, PortDataType::floating_point),
         port("decision", PortDirection::output, PortDataType::decision),
         port("passed", PortDirection::output, PortDataType::boolean)},
        nvision::workflow::ToolOrigin::built_in(),
        {},
        {ParameterDefinition(
            ParameterKey("threshold"),
            "Threshold",
            ParameterType::floating_point(NumericConstraint(0.0, 1.0)),
            true)});
}

// The only tool whose input accepts more than one source.
ToolTypeDefinition boolean_logic_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.logic.boolean-logic"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Boolean Logic", {"Logic and Results"}),
        {port("operands", PortDirection::input, PortDataType::boolean, true, true),
         port("result", PortDirection::output, PortDataType::boolean)});
}

// Produces many values on one port, which is what makes arity visible in a graph.
ToolTypeDefinition text_split_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.recognition.text-split"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Text Split", {"Recognition", "Text"}),
        {port("text", PortDirection::input, PortDataType::text_result),
         port("parts", PortDirection::output, PortDataType::text_result, true, true)});
}

ToolTypeDefinition text_consensus_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.tool.recognition.text-consensus"),
        SemanticVersion(1U, 0U, 0U),
        ToolCatalogMetadata("Text Consensus", {"Recognition", "Text"}),
        {port("candidates", PortDirection::input, PortDataType::text_result, true, true),
         port("text", PortDirection::output, PortDataType::text_result)});
}

WorkflowNode node(std::string id, std::string tool_type_id, NodeConfiguration configuration = {}) {
    return WorkflowNode(
        WorkflowNodeId(std::move(id)),
        ToolTypeId(std::move(tool_type_id)),
        SemanticVersion(1U, 0U, 0U),
        "Node",
        std::move(configuration));
}

WorkflowNode source_node() {
    return node("node.source", "nvision.tool.acquisition.image-source");
}

WorkflowNode crop_node() {
    return node(
        "node.crop",
        "nvision.tool.processing.crop",
        NodeConfiguration({NodeParameterAssignment(
            ParameterKey("roi"), ParameterValue::region_of_interest(sample_rectangle()))}));
}

WorkflowNode text_node() {
    return node(
        "node.text",
        "nvision.tool.recognition.text-recognition",
        NodeConfiguration({NodeParameterAssignment(
            ParameterKey("minimum-confidence"), ParameterValue::floating_point(0.8))}));
}

WorkflowNode compare_node(std::string id) {
    return node(
        std::move(id),
        "nvision.tool.logic.compare",
        NodeConfiguration({NodeParameterAssignment(
            ParameterKey("threshold"), ParameterValue::floating_point(0.7))}));
}

WorkflowConnection connect(
    std::string source_node_id,
    std::string source_port,
    std::string target_node_id,
    std::string target_port) {
    return WorkflowConnection(
        WorkflowNodeId(std::move(source_node_id)),
        PortKey(std::move(source_port)),
        WorkflowNodeId(std::move(target_node_id)),
        PortKey(std::move(target_port)));
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
    static_cast<void>(registry.register_definition(source_definition()));
    static_cast<void>(registry.register_definition(crop_definition()));
    static_cast<void>(registry.register_definition(text_definition()));
    static_cast<void>(registry.register_definition(sharpness_definition()));
    static_cast<void>(registry.register_definition(compare_definition()));
    static_cast<void>(registry.register_definition(boolean_logic_definition()));
    static_cast<void>(registry.register_definition(text_split_definition()));
    static_cast<void>(registry.register_definition(text_consensus_definition()));
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

    const WorkflowNode drifted = node(
        "node.text",
        "nvision.tool.recognition.text-recognition",
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

    const WorkflowNodeBinding missing =
        bind_workflow_node(node("node.compare", "nvision.tool.logic.compare"), registry);
    NVISION_REQUIRE(missing.is_resolved());
    NVISION_REQUIRE(missing.diagnostics().size() == 1U);
    NVISION_REQUIRE(missing.diagnostics()[0].code().value() == "MISSING_REQUIRED_PARAMETER");
    NVISION_REQUIRE(missing.diagnostics()[0].parameter_key()->value() == "threshold");

    // A required parameter that declares a default is already satisfied.
    const WorkflowNodeBinding defaulted =
        bind_workflow_node(node("node.crop-default", "nvision.tool.processing.crop"), registry);
    NVISION_REQUIRE(defaulted.diagnostics().empty());
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

void invalid_graph_topology_is_rejected() {
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowGraph invalid({}, {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowGraph invalid({crop_node(), crop_node()}, {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowConnection invalid = connect("node.crop", "image", "node.crop", "image");
        static_cast<void>(invalid);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowGraph invalid(
            {crop_node(), text_node()}, {connect("node.crop", "image", "node.absent", "image")});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        WorkflowGraph invalid(
            {crop_node(), text_node()},
            {connect("node.crop", "image", "node.text", "image"),
             connect("node.crop", "image", "node.text", "image")});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        // A cycle can never be ordered, so it is rejected without consulting a registry.
        WorkflowGraph invalid(
            {crop_node(), text_node()},
            {connect("node.crop", "image", "node.text", "image"),
             connect("node.text", "text", "node.crop", "image")});
    }));
}

void graph_binding_validates_typed_connections() {
    const ToolRegistry registry = reference_registry();

    const WorkflowGraph pipeline(
        {source_node(), crop_node(), text_node()},
        {connect("node.source", "image", "node.crop", "image"),
         connect("node.crop", "image", "node.text", "image")});

    const WorkflowGraphBinding binding = bind_workflow_graph(pipeline, registry);
    NVISION_REQUIRE(binding.is_fully_resolved());
    NVISION_REQUIRE(!binding.has_errors());
    NVISION_REQUIRE(binding.diagnostics().empty());
    NVISION_REQUIRE(binding.nodes().size() == 3U);

    // An unconnected required input is an error unless a caller declares it fed.
    const WorkflowGraph open_input({crop_node(), text_node()},
                                   {connect("node.crop", "image", "node.text", "image")});
    const WorkflowGraphBinding unfed = bind_workflow_graph(open_input, registry);
    NVISION_REQUIRE(unfed.has_errors());
    const auto* missing_input = find_diagnostic(unfed.diagnostics(), "MISSING_REQUIRED_INPUT");
    NVISION_REQUIRE(missing_input != nullptr);
    NVISION_REQUIRE(missing_input->node_id()->value() == "node.crop");
    NVISION_REQUIRE(missing_input->port_key()->value() == "image");

    const WorkflowGraphBinding fed = bind_workflow_graph(
        open_input, registry, {NodePortAddress(WorkflowNodeId("node.crop"), PortKey("image"))});
    NVISION_REQUIRE(!fed.has_errors());

    // A text result cannot feed a floating point input.
    const WorkflowGraph mismatched(
        {source_node(), text_node(), compare_node("node.compare")},
        {connect("node.source", "image", "node.text", "image"),
         connect("node.text", "text", "node.compare", "value")});
    const WorkflowGraphBinding mismatch_binding = bind_workflow_graph(mismatched, registry);
    NVISION_REQUIRE(mismatch_binding.has_errors());
    NVISION_REQUIRE(
        find_diagnostic(mismatch_binding.diagnostics(), "CONNECTION_TYPE_MISMATCH") != nullptr);

    const WorkflowGraph undeclared_port(
        {source_node(), crop_node()}, {connect("node.source", "frame", "node.crop", "image")});
    const WorkflowGraphBinding undeclared_binding =
        bind_workflow_graph(undeclared_port, registry);
    const auto* undeclared =
        find_diagnostic(undeclared_binding.diagnostics(), "MISSING_CONNECTED_PORT");
    NVISION_REQUIRE(undeclared != nullptr);
    NVISION_REQUIRE(undeclared->port_key()->value() == "frame");

    // A single-source input rejects a second edge; a multi-source input accepts one.
    const WorkflowGraph double_fed(
        {source_node(),
         node("node.sharpness-a", "nvision.tool.measurement.sharpness"),
         node("node.sharpness-b", "nvision.tool.measurement.sharpness"),
         compare_node("node.compare")},
        {connect("node.source", "image", "node.sharpness-a", "image"),
         connect("node.source", "image", "node.sharpness-b", "image"),
         connect("node.sharpness-a", "value", "node.compare", "value"),
         connect("node.sharpness-b", "value", "node.compare", "value")});
    const WorkflowGraphBinding double_binding = bind_workflow_graph(double_fed, registry);
    NVISION_REQUIRE(
        find_diagnostic(double_binding.diagnostics(), "PORT_ALREADY_CONNECTED") != nullptr);

    const WorkflowGraph multi_source(
        {source_node(),
         node("node.sharpness", "nvision.tool.measurement.sharpness"),
         compare_node("node.compare-a"),
         compare_node("node.compare-b"),
         node("node.logic", "nvision.tool.logic.boolean-logic")},
        {connect("node.source", "image", "node.sharpness", "image"),
         connect("node.sharpness", "value", "node.compare-a", "value"),
         connect("node.sharpness", "value", "node.compare-b", "value"),
         connect("node.compare-a", "passed", "node.logic", "operands"),
         connect("node.compare-b", "passed", "node.logic", "operands")});
    const WorkflowGraphBinding multi_binding = bind_workflow_graph(multi_source, registry);
    NVISION_REQUIRE(!multi_binding.has_errors());
    NVISION_REQUIRE(multi_binding.diagnostics().empty());
}

// A plural port is a different contract from a singular one, not a convenience. A graph
// that connects the two would validate today and lose values at execution tomorrow.
void collection_ports_declare_arity_in_the_graph() {
    const ToolRegistry registry = reference_registry();

    const WorkflowGraph flattened(
        {source_node(),
         text_node(),
         node("node.split-a", "nvision.tool.recognition.text-split"),
         node("node.split-b", "nvision.tool.recognition.text-split")},
        {connect("node.source", "image", "node.text", "image"),
         connect("node.text", "text", "node.split-a", "text"),
         // A collection cannot be poured into a port that holds one value.
         connect("node.split-a", "parts", "node.split-b", "text")});
    const WorkflowGraphBinding flattened_binding = bind_workflow_graph(flattened, registry);
    NVISION_REQUIRE(flattened_binding.has_errors());
    const auto* arity = find_diagnostic(flattened_binding.diagnostics(), "PORT_ARITY_MISMATCH");
    NVISION_REQUIRE(arity != nullptr);
    NVISION_REQUIRE(arity->node_id()->value() == "node.split-b");
    NVISION_REQUIRE(arity->port_key()->value() == "text");

    // A collection input gathers whatever reaches it: a plural source, a singular one, or
    // both, in connection order.
    const WorkflowGraph gathered(
        {source_node(),
         text_node(),
         node("node.split", "nvision.tool.recognition.text-split"),
         node("node.consensus", "nvision.tool.recognition.text-consensus")},
        {connect("node.source", "image", "node.text", "image"),
         connect("node.text", "text", "node.split", "text"),
         connect("node.split", "parts", "node.consensus", "candidates"),
         connect("node.text", "text", "node.consensus", "candidates")});
    const WorkflowGraphBinding gathered_binding = bind_workflow_graph(gathered, registry);
    NVISION_REQUIRE(!gathered_binding.has_errors());
    NVISION_REQUIRE(gathered_binding.diagnostics().empty());
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
            nvision::workflow::ToolOrigin::built_in(),
            {},
            {roi_parameter(), confidence_parameter()}),
        WorkflowGraph(
            {crop_node(), text_node()}, {connect("node.crop", "image", "node.text", "image")}),
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
    NVISION_REQUIRE(subworkflow.graph().nodes().size() == 2U);
    NVISION_REQUIRE(subworkflow.graph().connections().size() == 1U);
    NVISION_REQUIRE(subworkflow.graph().find_node(WorkflowNodeId("node.text")) != nullptr);
    NVISION_REQUIRE(subworkflow.graph().find_node(WorkflowNodeId("node.absent")) == nullptr);

    // The interface mapping satisfies the internal required input that no edge feeds.
    const WorkflowGraphBinding binding = bind_subworkflow(subworkflow, registry);
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
        WorkflowGraph({crop_node(), text_node()}, {}),
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

    const WorkflowGraph graph(
        {crop_node(), text_node()}, {connect("node.crop", "image", "node.text", "image")});
    const SubworkflowPortMapping image_mapping(
        PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"), PortKey("image"));
    const SubworkflowPortMapping text_mapping(
        PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"), PortKey("text"));

    NVISION_REQUIRE(rejects_invalid_argument([&interface_definition, &image_mapping] {
        // A subworkflow that contains itself can never be expanded.
        SubworkflowDefinition invalid(
            interface_definition,
            WorkflowGraph(
                {crop_node(),
                 node("node.self", "nvision.subworkflow.recognition.region-text")},
                {}),
            {image_mapping,
             SubworkflowPortMapping(
                 PortKey("text"), PortDirection::output, WorkflowNodeId("node.self"),
                 PortKey("text"))});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&interface_definition, &graph, &image_mapping] {
        // The declared output port is never produced.
        SubworkflowDefinition invalid(interface_definition, graph, {image_mapping});
    }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &graph, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                graph,
                {image_mapping,
                 text_mapping,
                 SubworkflowPortMapping(
                     PortKey("text"), PortDirection::output, WorkflowNodeId("node.crop"),
                     PortKey("image"))});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &graph, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition, graph, {image_mapping, image_mapping, text_mapping});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &graph, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                graph,
                {image_mapping,
                 text_mapping,
                 SubworkflowPortMapping(
                     PortKey("mask"), PortDirection::input, WorkflowNodeId("node.crop"),
                     PortKey("mask"))});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &graph, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                graph,
                {image_mapping,
                 text_mapping,
                 SubworkflowPortMapping(
                     PortKey("image"), PortDirection::input, WorkflowNodeId("node.absent"),
                     PortKey("image"))});
        }));
    NVISION_REQUIRE(
        rejects_invalid_argument([&interface_definition, &graph, &image_mapping, &text_mapping] {
            SubworkflowDefinition invalid(
                interface_definition,
                graph,
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
        nvision::workflow::ToolOrigin::built_in(),
        {},
        {roi_parameter()});

    NVISION_REQUIRE(rejects_invalid_argument([&with_roi, &graph, &image_mapping, &text_mapping] {
        SubworkflowDefinition invalid(
            with_roi,
            graph,
            {image_mapping, text_mapping},
            {SubworkflowParameterPromotion(
                 ParameterKey("roi"), WorkflowNodeId("node.crop"), ParameterKey("roi")),
             SubworkflowParameterPromotion(
                 ParameterKey("roi"), WorkflowNodeId("node.text"), ParameterKey("roi"))});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&with_roi, &graph, &image_mapping, &text_mapping] {
        SubworkflowDefinition invalid(
            with_roi,
            graph,
            {image_mapping, text_mapping},
            {SubworkflowParameterPromotion(
                ParameterKey("roi"), WorkflowNodeId("node.absent"), ParameterKey("roi"))});
    }));
}

void subworkflow_binding_reports_contract_mismatches() {
    const ToolRegistry registry = reference_registry();
    const WorkflowGraph graph(
        {crop_node(), text_node()}, {connect("node.crop", "image", "node.text", "image")});

    // The interface promises a decision, but the mapped node port produces text.
    const SubworkflowDefinition wrong_type(
        ToolTypeDefinition(
            ToolTypeId("nvision.subworkflow.recognition.wrong-type"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Wrong Type", {"Recognition"}),
            {port("image", PortDirection::input, PortDataType::image_view),
             port("text", PortDirection::output, PortDataType::decision)}),
        graph,
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});

    const WorkflowGraphBinding type_binding = bind_subworkflow(wrong_type, registry);
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
        graph,
        {SubworkflowPortMapping(
             PortKey("mask"), PortDirection::input, WorkflowNodeId("node.crop"), PortKey("mask")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});

    const WorkflowGraphBinding port_binding = bind_subworkflow(missing_port, registry);
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
            nvision::workflow::ToolOrigin::built_in(),
            {},
            {roi_parameter(),
             ParameterDefinition(
                 ParameterKey("orientation-steps"),
                 "Orientation Steps",
                 ParameterType::integer(NumericConstraint(1.0, 4.0)),
                 true,
                 ParameterValue::integer(2))}),
        graph,
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

    const WorkflowGraphBinding promotion_binding = bind_subworkflow(wrong_promotion, registry);
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
            nvision::workflow::ToolOrigin::built_in(),
            {},
            {roi_parameter()}),
        graph,
        {SubworkflowPortMapping(
             PortKey("image"), PortDirection::input, WorkflowNodeId("node.crop"),
             PortKey("image")),
         SubworkflowPortMapping(
             PortKey("text"), PortDirection::output, WorkflowNodeId("node.text"),
             PortKey("text"))});

    const WorkflowGraphBinding unpromoted_binding = bind_subworkflow(unpromoted, registry);
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
    const WorkflowGraphBinding binding = bind_subworkflow(subworkflow, registry);

    NVISION_REQUIRE(!binding.is_fully_resolved());
    NVISION_REQUIRE(binding.has_errors());
    NVISION_REQUIRE(!binding.find_node(WorkflowNodeId("node.crop"))->is_resolved());
    NVISION_REQUIRE(binding.find_node(WorkflowNodeId("node.text"))->is_resolved());

    // Only the missing tool is reported. Its edge, mapping, and promotion stay silent, and
    // the input it feeds is still counted as connected.
    NVISION_REQUIRE(binding.diagnostics().empty());
    NVISION_REQUIRE(
        binding.find_node(WorkflowNodeId("node.crop"))->diagnostics()[0].code().value() ==
        "UNKNOWN_TOOL_TYPE");
    NVISION_REQUIRE(binding.find_node(WorkflowNodeId("node.crop"))
                        ->node()
                        .configuration()
                        .contains(ParameterKey("roi")));
}

}  // namespace

int main() {
    unresolved_nodes_keep_identity_and_configuration();
    resolved_nodes_report_schema_problems_without_dropping_values();
    invalid_node_contracts_are_rejected();
    invalid_graph_topology_is_rejected();
    graph_binding_validates_typed_connections();
    collection_ports_declare_arity_in_the_graph();
    subworkflows_expose_a_reusable_graph_through_a_tool_interface();
    invalid_subworkflow_structures_are_rejected();
    subworkflow_binding_reports_contract_mismatches();
    unresolved_nodes_do_not_hide_behind_mapping_diagnostics();
    return EXIT_SUCCESS;
}
