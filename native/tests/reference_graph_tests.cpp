// Validates the two reference applications of the execution plan against the generic
// built-in catalog. The application names appear only here, in test data. Nothing in this
// file introduces a production type, and the graphs use no tool that carries product
// vocabulary.
#include "nvision/workflow/builtin_catalog.hpp"
#include "nvision/workflow/workflow_graph.hpp"

#include <cstdio>
#include <cstdlib>
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
using nvision::core::IntegrityAlgorithm;
using nvision::core::IntegrityDigest;
using nvision::workflow::NodeConfiguration;
using nvision::workflow::NodeParameterAssignment;
using nvision::core::ParameterKey;
using nvision::workflow::ParameterValue;
using nvision::core::PortKey;
using nvision::core::ResourceId;
using nvision::core::ResourceIntegrity;
using nvision::core::ResourceKind;
using nvision::core::ResourceReference;
using nvision::core::SemanticVersion;
using nvision::core::ToolTypeId;
using nvision::workflow::WorkflowConnection;
using nvision::workflow::WorkflowGraph;
using nvision::workflow::WorkflowGraphBinding;
using nvision::workflow::WorkflowNode;
using nvision::core::WorkflowNodeId;

ResourceReference reference(std::string id, const ResourceKind kind) {
    return ResourceReference(
        ResourceId(std::move(id)),
        SemanticVersion(1U, 0U, 0U),
        kind,
        ResourceIntegrity(
            IntegrityDigest(IntegrityAlgorithm::sha256, std::string(64U, 'a')), 4096U));
}

NodeParameterAssignment assign(std::string key, ParameterValue value) {
    return NodeParameterAssignment(ParameterKey(std::move(key)), std::move(value));
}

ParameterValue strings(const std::vector<std::string>& values) {
    std::vector<ParameterValue> items;
    items.reserve(values.size());
    for (const auto& value : values) {
        items.push_back(ParameterValue::string(value));
    }
    return ParameterValue::array(std::move(items));
}

ParameterValue floats(const std::vector<double>& values) {
    std::vector<ParameterValue> items;
    items.reserve(values.size());
    for (const double value : values) {
        items.push_back(ParameterValue::floating_point(value));
    }
    return ParameterValue::array(std::move(items));
}

ParameterValue rectangle() {
    return ParameterValue::region_of_interest(
        AxisAlignedRectangle(CoordinateSpaceId("image.pixel"), 16.0, 24.0, 320.0, 180.0));
}

WorkflowNode node(
    std::string id,
    std::string tool_type_id,
    std::vector<NodeParameterAssignment> assignments = {}) {
    std::string display = id;
    return WorkflowNode(
        WorkflowNodeId(std::move(id)),
        ToolTypeId(std::move(tool_type_id)),
        SemanticVersion(1U, 0U, 0U),
        std::move(display),
        NodeConfiguration(std::move(assignments)));
}

WorkflowConnection edge(
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

void report(const WorkflowGraphBinding& binding) {
    // Keeps a failing assertion debuggable without a test framework.
    for (const auto& node_binding : binding.nodes()) {
        for (const auto& diagnostic : node_binding.diagnostics()) {
            std::fputs(
                (node_binding.node().id().value() + ": " + diagnostic.code().value() + ": " +
                 diagnostic.detail() + "\n")
                    .c_str(),
                stderr);
        }
    }
    for (const auto& diagnostic : binding.diagnostics()) {
        std::fputs(
            (diagnostic.code().value() + ": " + diagnostic.detail() + "\n").c_str(), stderr);
    }
}

void require_clean(const WorkflowGraphBinding& binding, const std::size_t expected_nodes) {
    if (!binding.is_fully_resolved() || binding.has_errors()) {
        report(binding);
    }
    NVISION_REQUIRE(binding.is_fully_resolved());
    NVISION_REQUIRE(!binding.has_errors());
    NVISION_REQUIRE(binding.nodes().size() == expected_nodes);
}

// Execution plan Section 12.1. Two capture roles, per-lane detection and multi-orientation
// recognition, normalization, consensus, a Recipe-supplied lookup, print quality scoring,
// pattern validation, and one combined decision.
void text_inspection_reference_graph_validates() {
    const auto registry = nvision::workflow::create_builtin_registry();

    const WorkflowGraph graph(
        {node("node.capture",
              "nvision.tool.acquisition.capture-sequence",
              {assign("capture-roles", strings({"I1", "I2"}))}),
         node("node.image-1",
              "nvision.tool.acquisition.image-buffer",
              {assign("capture-role", ParameterValue::string("I1"))}),
         node("node.image-2",
              "nvision.tool.acquisition.image-buffer",
              {assign("capture-role", ParameterValue::string("I2"))}),
         node("node.detect-1",
              "nvision.tool.recognition.text-detection",
              {assign(
                  "model",
                  ParameterValue::model_reference(
                      reference("resource.text-detector", ResourceKind::model)))}),
         node("node.detect-2",
              "nvision.tool.recognition.text-detection",
              {assign(
                  "model",
                  ParameterValue::model_reference(
                      reference("resource.text-detector", ResourceKind::model)))}),
         node("node.recognize-1",
              "nvision.tool.recognition.text-recognition",
              {assign(
                   "model",
                   ParameterValue::model_reference(
                       reference("resource.text-recognizer", ResourceKind::model))),
               assign(
                   "dictionary",
                   ParameterValue::resource_reference(
                       reference("resource.character-dictionary", ResourceKind::dictionary))),
               assign("orientations", floats({0.0, 180.0}))}),
         node("node.recognize-2",
              "nvision.tool.recognition.text-recognition",
              {assign(
                   "model",
                   ParameterValue::model_reference(
                       reference("resource.text-recognizer", ResourceKind::model))),
               assign("orientations", floats({0.0, 180.0}))}),
         node("node.normalize-1", "nvision.tool.recognition.text-normalize"),
         node("node.normalize-2", "nvision.tool.recognition.text-normalize"),
         node("node.split-1",
              "nvision.tool.recognition.text-split",
              {assign("separator", ParameterValue::string("/"))}),
         node("node.split-2",
              "nvision.tool.recognition.text-split",
              {assign("separator", ParameterValue::string("/"))}),
         node("node.similarity", "nvision.tool.recognition.string-similarity"),
         node("node.consensus", "nvision.tool.recognition.text-consensus"),
         node("node.type-lookup",
              "nvision.tool.logic.lookup-table",
              {assign("keys", strings({"upright-upright", "upright-inverted"})),
               assign("values", strings({"type-1", "type-2"}))}),
         node("node.quality-sharpness", "nvision.tool.measurement.sharpness"),
         node("node.quality-contrast", "nvision.tool.measurement.contrast"),
         node("node.quality-edges", "nvision.tool.measurement.edge-density"),
         node("node.quality-score",
              "nvision.tool.logic.weighted-score",
              {assign("weights", floats({0.5, 0.3, 0.2}))}),
         node("node.quality-compare",
              "nvision.tool.logic.compare",
              {assign("threshold", ParameterValue::floating_point(0.75))}),
         node("node.pattern-validate",
              "nvision.tool.recognition.regex-validate",
              {assign("pattern", ParameterValue::string("^[A-Z0-9]+/[A-Z0-9]+$"))}),
         node("node.logic", "nvision.tool.logic.boolean-logic"),
         node("node.decide", "nvision.tool.logic.decision"),
         node("node.evidence", "nvision.tool.logic.evidence-output"),
         node("node.result", "nvision.tool.logic.result-builder")},
        {edge("node.capture", "images", "node.image-1", "images"),
         edge("node.capture", "images", "node.image-2", "images"),
         edge("node.image-1", "image", "node.detect-1", "image"),
         edge("node.image-2", "image", "node.detect-2", "image"),
         edge("node.image-1", "image", "node.recognize-1", "image"),
         edge("node.image-2", "image", "node.recognize-2", "image"),
         edge("node.detect-1", "regions", "node.recognize-1", "regions"),
         edge("node.detect-2", "regions", "node.recognize-2", "regions"),
         edge("node.recognize-1", "text", "node.normalize-1", "text"),
         edge("node.recognize-2", "text", "node.normalize-2", "text"),
         edge("node.normalize-1", "text", "node.split-1", "text"),
         edge("node.normalize-2", "text", "node.split-2", "text"),
         edge("node.normalize-1", "text", "node.similarity", "left"),
         edge("node.normalize-2", "text", "node.similarity", "right"),
         edge("node.split-1", "parts", "node.consensus", "candidates"),
         edge("node.split-2", "parts", "node.consensus", "candidates"),
         edge("node.consensus", "value", "node.type-lookup", "key"),
         edge("node.recognize-1", "crops", "node.quality-sharpness", "image"),
         edge("node.recognize-1", "crops", "node.quality-contrast", "image"),
         edge("node.recognize-1", "crops", "node.quality-edges", "image"),
         edge("node.quality-sharpness", "measurement", "node.quality-score", "values"),
         edge("node.quality-contrast", "measurement", "node.quality-score", "values"),
         edge("node.quality-edges", "measurement", "node.quality-score", "values"),
         edge("node.quality-score", "score", "node.quality-compare", "value"),
         edge("node.normalize-1", "text", "node.pattern-validate", "text"),
         edge("node.quality-compare", "passed", "node.logic", "operands"),
         edge("node.pattern-validate", "valid", "node.logic", "operands"),
         edge("node.type-lookup", "found", "node.logic", "operands"),
         edge("node.logic", "result", "node.decide", "passed"),
         edge("node.image-1", "image", "node.evidence", "image"),
         edge("node.decide", "decision", "node.result", "decision"),
         edge("node.quality-sharpness", "measurement", "node.result", "measurements"),
         edge("node.evidence", "evidence", "node.result", "evidence")});

    require_clean(bind_workflow_graph(graph, registry), 24U);
}

// Execution plan Section 12.2. Golden segmentation, seed extraction, row structure, and a
// learned point grid published as a versioned resource.
void point_grid_training_reference_graph_validates() {
    const auto registry = nvision::workflow::create_builtin_registry();

    const WorkflowGraph graph(
        {node("node.source", "nvision.tool.acquisition.image-source"),
         node("node.debayer",
              "nvision.tool.processing.color-convert",
              {assign(
                  "conversion",
                  ParameterValue::enumeration(
                      nvision::core::EnumOptionKey("bayer-gr-to-bgr")))}),
         node("node.crop", "nvision.tool.processing.crop", {assign("roi", rectangle())}),
         node("node.hsv",
              "nvision.tool.processing.color-convert",
              {assign(
                  "conversion",
                  ParameterValue::enumeration(nvision::core::EnumOptionKey("bgr-to-hsv")))}),
         node("node.range",
              "nvision.tool.processing.color-range",
              {assign("lower", floats({20.0, 80.0, 60.0})),
               assign("upper", floats({40.0, 255.0, 255.0}))}),
         node("node.morph", "nvision.tool.processing.morphology"),
         node("node.distance", "nvision.tool.processing.distance-transform"),
         node("node.binarize", "nvision.tool.processing.threshold"),
         node("node.contours", "nvision.tool.geometry.contour-extraction"),
         node("node.circles", "nvision.tool.geometry.minimum-enclosing-circle"),
         node("node.cluster", "nvision.tool.structure.dbscan"),
         node("node.fit", "nvision.tool.geometry.line-fit"),
         node("node.sort", "nvision.tool.structure.sort-points-along-line"),
         node("node.learn", "nvision.tool.structure.point-grid-learn")},
        {edge("node.source", "image", "node.debayer", "image"),
         edge("node.debayer", "image", "node.crop", "image"),
         edge("node.crop", "image", "node.hsv", "image"),
         edge("node.hsv", "image", "node.range", "image"),
         edge("node.range", "mask", "node.morph", "mask"),
         edge("node.morph", "mask", "node.distance", "mask"),
         edge("node.distance", "image", "node.binarize", "image"),
         edge("node.binarize", "mask", "node.contours", "mask"),
         edge("node.contours", "contours", "node.circles", "contours"),
         edge("node.circles", "centers", "node.cluster", "points"),
         edge("node.cluster", "centers", "node.fit", "points"),
         edge("node.cluster", "centers", "node.sort", "points"),
         edge("node.fit", "line", "node.sort", "line"),
         edge("node.sort", "points", "node.learn", "points")});

    require_clean(bind_workflow_graph(graph, registry), 14U);
}

// Execution plan Section 12.3. Feature alignment against a golden image, verification
// scoring, the same segmentation chain, and grid inspection against a bound resource.
void point_grid_inspection_reference_graph_validates() {
    const auto registry = nvision::workflow::create_builtin_registry();

    const WorkflowGraph graph(
        {node("node.source", "nvision.tool.acquisition.image-source"),
         node("node.template",
              "nvision.tool.acquisition.image-source",
              {assign("capture-role", ParameterValue::string("golden"))}),
         node("node.debayer",
              "nvision.tool.processing.color-convert",
              {assign(
                  "conversion",
                  ParameterValue::enumeration(
                      nvision::core::EnumOptionKey("bayer-gr-to-bgr")))}),
         node("node.match", "nvision.tool.geometry.feature-detect-and-match"),
         node("node.homography", "nvision.tool.geometry.homography-estimate"),
         node("node.validate", "nvision.tool.geometry.homography-validate"),
         node("node.warp", "nvision.tool.processing.perspective-warp"),
         node("node.refine", "nvision.tool.geometry.ecc-refine"),
         node("node.ncc", "nvision.tool.measurement.ncc"),
         node("node.hsv",
              "nvision.tool.processing.color-convert",
              {assign(
                  "conversion",
                  ParameterValue::enumeration(nvision::core::EnumOptionKey("bgr-to-hsv")))}),
         node("node.range", "nvision.tool.processing.color-range"),
         node("node.morph", "nvision.tool.processing.morphology"),
         node("node.distance", "nvision.tool.processing.distance-transform"),
         node("node.binarize", "nvision.tool.processing.threshold"),
         node("node.contours", "nvision.tool.geometry.contour-extraction"),
         node("node.circles", "nvision.tool.geometry.minimum-enclosing-circle"),
         node("node.inspect",
              "nvision.tool.structure.point-grid-inspect",
              {assign(
                  "model",
                  ParameterValue::resource_reference(
                      reference("resource.point-grid", ResourceKind::template_asset)))}),
         node("node.ncc-compare",
              "nvision.tool.logic.compare",
              {assign("threshold", ParameterValue::floating_point(0.8))}),
         node("node.logic", "nvision.tool.logic.boolean-logic"),
         node("node.decide", "nvision.tool.logic.decision"),
         node("node.evidence", "nvision.tool.logic.evidence-output"),
         node("node.result", "nvision.tool.logic.result-builder")},
        {edge("node.source", "image", "node.debayer", "image"),
         edge("node.debayer", "image", "node.match", "image"),
         edge("node.template", "image", "node.match", "reference"),
         edge("node.match", "matches", "node.homography", "matches"),
         edge("node.homography", "homography", "node.validate", "homography"),
         edge("node.debayer", "image", "node.warp", "image"),
         edge("node.homography", "homography", "node.warp", "homography"),
         edge("node.warp", "image", "node.refine", "image"),
         edge("node.template", "image", "node.refine", "reference"),
         edge("node.warp", "transform", "node.refine", "transform"),
         edge("node.warp", "image", "node.ncc", "image"),
         edge("node.template", "image", "node.ncc", "reference"),
         edge("node.warp", "image", "node.hsv", "image"),
         edge("node.hsv", "image", "node.range", "image"),
         edge("node.range", "mask", "node.morph", "mask"),
         edge("node.morph", "mask", "node.distance", "mask"),
         edge("node.distance", "image", "node.binarize", "image"),
         edge("node.binarize", "mask", "node.contours", "mask"),
         edge("node.contours", "contours", "node.circles", "contours"),
         edge("node.circles", "centers", "node.inspect", "points"),
         edge("node.ncc", "score", "node.ncc-compare", "value"),
         edge("node.ncc-compare", "passed", "node.logic", "operands"),
         edge("node.validate", "valid", "node.logic", "operands"),
         edge("node.logic", "result", "node.decide", "passed"),
         edge("node.warp", "image", "node.evidence", "image"),
         edge("node.decide", "decision", "node.result", "decision"),
         edge("node.evidence", "evidence", "node.result", "evidence")});

    require_clean(bind_workflow_graph(graph, registry), 22U);
}

// The catalog is shared, so a change that only one application needs would break the other.
void both_reference_graphs_share_one_catalog() {
    const auto registry = nvision::workflow::create_builtin_registry();
    const auto used = [&registry](const std::string& id) {
        return registry.resolve_exact(ToolTypeId(id), SemanticVersion(1U, 0U, 0U)).is_resolved();
    };

    NVISION_REQUIRE(used("nvision.tool.recognition.text-consensus"));
    NVISION_REQUIRE(used("nvision.tool.structure.point-grid-inspect"));
    NVISION_REQUIRE(used("nvision.tool.processing.color-convert"));
    NVISION_REQUIRE(used("nvision.tool.logic.decision"));
}

}  // namespace

int main() {
    text_inspection_reference_graph_validates();
    point_grid_training_reference_graph_validates();
    point_grid_inspection_reference_graph_validates();
    both_reference_graphs_share_one_catalog();
    return EXIT_SUCCESS;
}
