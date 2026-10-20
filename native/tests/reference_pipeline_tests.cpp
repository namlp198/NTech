// Proves the M3 exit-gate claim that the Pin Insert source algorithms can be represented
// without a Pin Insert engine class.
//
// The training and inspection flows of Section 12 of the execution plan are run here as a
// chain of ordinary catalog tools on a generated fixture. Nothing in this file names a
// product, a pin, or a connector: the platform sees colour, regions, contours, circles,
// points, a grid and a deviation, and only the reference project would give those meaning.

#include "nvision/acquisition/image.hpp"
#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/core/point_grid.hpp"
#include "nvision/vision/builtin_tool_implementations.hpp"
#include "nvision/workflow/builtin_catalog.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/port_value.hpp"
#include "nvision/workflow/tool_execution.hpp"
#include "nvision/workflow/tool_implementation_registry.hpp"
#include "nvision/workflow/workflow_node.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define NVISION_REQUIRE(condition)                                               \
    do {                                                                         \
        if (!(condition)) {                                                      \
            std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            std::abort();                                                        \
        }                                                                        \
    } while (false)

namespace {

using nvision::acquisition::Image;
using nvision::acquisition::ImageView;
using nvision::acquisition::PixelFormat;
using nvision::core::CoordinateSpaceId;
using nvision::core::EnumOptionKey;
using nvision::core::ExecutionSessionId;
using nvision::core::ImageId;
using nvision::core::ParameterKey;
using nvision::core::PointGridModel;
using nvision::core::PortKey;
using nvision::core::SemanticVersion;
using nvision::core::ToolTypeId;
using nvision::core::Transform2D;
using nvision::core::WorkflowNodeId;
using nvision::vision::create_vision_implementation_registry;
using nvision::workflow::create_builtin_registry;
using nvision::workflow::execute_tool;
using nvision::workflow::NodeConfiguration;
using nvision::workflow::NodeParameterAssignment;
using nvision::workflow::ParameterBinding;
using nvision::workflow::ParameterValue;
using nvision::workflow::PortValue;
using nvision::workflow::PortValueMap;
using nvision::workflow::ToolExecutionContext;
using nvision::workflow::ToolExecutionOutcome;
using nvision::workflow::ToolImplementationRegistry;
using nvision::workflow::ToolRegistry;

constexpr std::uint32_t image_width = 64U;
constexpr std::uint32_t image_height = 48U;
constexpr std::uint32_t feature_size = 5U;

// Six bright regions in two rows of three, offset by the given shift. A generated fixture
// keeps the expected geometry exact and adds no asset whose provenance would need
// resolving before it could be redistributed.
ImageView layout(const std::string& identity, const std::uint32_t shift_x, const std::uint32_t shift_y) {
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(image_width) * image_height * 3U, 0U);
    const std::array<std::uint32_t, 3> columns{10U, 25U, 40U};
    const std::array<std::uint32_t, 2> rows{10U, 30U};
    for (const std::uint32_t row : rows) {
        for (const std::uint32_t column : columns) {
            for (std::uint32_t y = 0U; y < feature_size; ++y) {
                for (std::uint32_t x = 0U; x < feature_size; ++x) {
                    const std::size_t offset =
                        (static_cast<std::size_t>(row + shift_y + y) * image_width +
                         (column + shift_x + x)) *
                        3U;
                    // Pure green in BGR order, which becomes a narrow hue band in HSV.
                    pixels[offset + 1U] = 255U;
                }
            }
        }
    }
    return ImageView(std::make_shared<const Image>(
        ImageId(identity), image_width, image_height, image_width * 3U, PixelFormat::bgr8,
        std::move(pixels)));
}

const SemanticVersion& initial_version() {
    static const SemanticVersion version(1U, 0U, 0U);
    return version;
}

class Pipeline final {
public:
    Pipeline()
        : definitions_(create_builtin_registry()),
          implementations_(create_vision_implementation_registry()) {}

    [[nodiscard]] ToolExecutionOutcome run(
        const std::string& tool_id,
        const std::string& node,
        PortValueMap inputs,
        NodeConfiguration configuration = {}) const {
        const auto binding =
            implementations_.bind(ToolTypeId(tool_id), initial_version(), definitions_);
        // Every step resolves out of the one generic catalog, which is the claim itself.
        NVISION_REQUIRE(binding.is_bound());
        const ToolExecutionContext context(
            binding.definition(),
            ExecutionSessionId("session.pin-grid"),
            WorkflowNodeId(node),
            std::move(inputs),
            ParameterBinding::resolve(*binding.definition(), configuration));
        const ToolExecutionOutcome outcome = execute_tool(*binding.implementation(), context);
        NVISION_REQUIRE(outcome.is_success());
        return outcome;
    }

private:
    ToolRegistry definitions_;
    ToolImplementationRegistry implementations_;
};

NodeParameterAssignment option(const std::string& key, const std::string& value) {
    return NodeParameterAssignment(
        ParameterKey(key), ParameterValue::enumeration(EnumOptionKey(value)));
}

NodeParameterAssignment number(const std::string& key, const double value) {
    return NodeParameterAssignment(ParameterKey(key), ParameterValue::floating_point(value));
}

NodeParameterAssignment integer(const std::string& key, const std::int64_t value) {
    return NodeParameterAssignment(ParameterKey(key), ParameterValue::integer(value));
}

NodeParameterAssignment bounds(const std::string& key, const std::vector<double>& values) {
    std::vector<ParameterValue> items;
    items.reserve(values.size());
    for (const double value : values) {
        items.push_back(ParameterValue::floating_point(value));
    }
    return NodeParameterAssignment(ParameterKey(key), ParameterValue::array(std::move(items)));
}

PortValueMap image_input(const ImageView& image) {
    PortValueMap inputs;
    inputs.set(PortKey("image"), PortValue::image(image));
    return inputs;
}

PortValueMap mask_input(const ImageView& mask) {
    PortValueMap inputs;
    inputs.set(PortKey("mask"), PortValue::mask(mask));
    return inputs;
}

// What the located regions are and which raster they were located in.
struct Located final {
    PortValue centers;
    ImageView mask;
};

// Colour segmentation through to circle centres: the shared front half of both the Pin
// Insert training graph and its inspection graph, expressed only in catalog tools.
Located locate_features(const Pipeline& pipeline, const std::string& prefix, const ImageView& colour) {
    const ToolExecutionOutcome hsv = pipeline.run(
        "nvision.tool.processing.color-convert",
        prefix + ".hsv",
        image_input(colour),
        NodeConfiguration({option("conversion", "bgr-to-hsv")}));
    const ImageView& in_hue = hsv.outputs().find(PortKey("image"))->image_value();

    const ToolExecutionOutcome segmented = pipeline.run(
        "nvision.tool.processing.color-range",
        prefix + ".range",
        image_input(in_hue),
        NodeConfiguration(
            {bounds("lower", {40.0, 100.0, 100.0}), bounds("upper", {80.0, 255.0, 255.0})}));
    const ImageView& selected = segmented.outputs().find(PortKey("mask"))->image_value();

    const ToolExecutionOutcome cleaned = pipeline.run(
        "nvision.tool.processing.morphology",
        prefix + ".open",
        mask_input(selected),
        NodeConfiguration(
            {option("operation", "open"), integer("kernel-size", 3), integer("iterations", 1)}));
    const ImageView& cleaned_mask = cleaned.outputs().find(PortKey("mask"))->image_value();

    const ToolExecutionOutcome traced = pipeline.run(
        "nvision.tool.geometry.contour-extraction",
        prefix + ".contours",
        mask_input(cleaned_mask),
        NodeConfiguration({option("mode", "external"), number("minimum-area", 1.0)}));

    PortValueMap circle_inputs;
    circle_inputs.set(PortKey("contours"), *traced.outputs().find(PortKey("contours")));
    const ToolExecutionOutcome circles = pipeline.run(
        "nvision.tool.geometry.minimum-enclosing-circle", prefix + ".circles", std::move(circle_inputs));

    return Located{*circles.outputs().find(PortKey("centers")), cleaned_mask};
}

void the_reference_grid_pipelines_run_on_generic_tools_alone() {
    const Pipeline pipeline;

    // Training: locate the features of a golden raster and learn the layout they form.
    const Located golden = locate_features(pipeline, "golden", layout("fixture.golden", 0U, 0U));
    NVISION_REQUIRE(golden.centers.point_list_value().size() == 6U);

    PortValueMap learn_inputs;
    learn_inputs.set(PortKey("points"), golden.centers);
    const ToolExecutionOutcome learned = pipeline.run(
        "nvision.tool.structure.point-grid-learn",
        "golden.grid",
        std::move(learn_inputs),
        NodeConfiguration({number("row-tolerance", 5.0), number("column-tolerance", 2.0)}));

    const PointGridModel& model =
        learned.outputs().find(PortKey("model"))->point_grid_model_value();
    NVISION_REQUIRE(model.row_count() == 2U);
    NVISION_REQUIRE(model.point_count() == 6U);
    NVISION_REQUIRE(model.coordinate_space() == golden.mask.coordinate_space());

    // Inspection: locate the same features in a raster whose content sits two across and
    // one down from the golden one.
    const Located live = locate_features(pipeline, "live", layout("fixture.live", 2U, 1U));
    NVISION_REQUIRE(live.centers.point_list_value().size() == 6U);
    // The two runs produced different rasters, so their results are in different spaces and
    // cannot be compared until one is explicitly brought into the other.
    NVISION_REQUIRE(live.mask.coordinate_space() != golden.mask.coordinate_space());

    // In production this mapping comes from feature matching and a homography estimate.
    // Here it is stated outright so the expected deviation stays exact.
    PortValueMap align_inputs;
    align_inputs.set(PortKey("points"), live.centers);
    align_inputs.set(
        PortKey("transform"),
        PortValue::transform(Transform2D(
            live.mask.coordinate_space(),
            golden.mask.coordinate_space(),
            std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0})));
    const ToolExecutionOutcome aligned = pipeline.run(
        "nvision.tool.geometry.point-transform", "live.align", std::move(align_inputs));

    PortValueMap deviation_inputs;
    deviation_inputs.set(PortKey("points"), *aligned.outputs().find(PortKey("points")));
    deviation_inputs.set(PortKey("reference"), PortValue::point_grid_model(model));
    const ToolExecutionOutcome deviated = pipeline.run(
        "nvision.tool.measurement.grid-deviation", "live.deviation", std::move(deviation_inputs));

    // Every expected position is off by the shift that was applied to the fixture.
    const double maximum =
        deviated.outputs().find(PortKey("maximum"))->floating_point_value();
    NVISION_REQUIRE(std::abs(maximum - std::sqrt(5.0)) < 0.01);
    NVISION_REQUIRE(
        deviated.outputs().find(PortKey("measurements"))->measurement_list_value().size() == 6U);
}

void no_step_of_the_pipeline_needed_an_application_specific_tool() {
    const ToolRegistry definitions = create_builtin_registry();
    // The identities the pipeline above resolved, listed so that introducing a tool named
    // after a product to make the flow work would be visible rather than quiet.
    constexpr std::array<const char*, 8> used{
        "nvision.tool.processing.color-convert",
        "nvision.tool.processing.color-range",
        "nvision.tool.processing.morphology",
        "nvision.tool.geometry.contour-extraction",
        "nvision.tool.geometry.minimum-enclosing-circle",
        "nvision.tool.structure.point-grid-learn",
        "nvision.tool.geometry.point-transform",
        "nvision.tool.measurement.grid-deviation",
    };

    for (const char* identity : used) {
        const auto resolution =
            definitions.resolve_exact(ToolTypeId(identity), initial_version());
        NVISION_REQUIRE(resolution.is_resolved());
        NVISION_REQUIRE(
            resolution.definition()->origin().kind() ==
            nvision::workflow::ToolOriginKind::built_in);
    }
}

void run(const char* name, void (*test)()) {
    try {
        test();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        std::abort();
    }
}

}  // namespace

#define NVISION_RUN(test) run(#test, test)

int main() {
    NVISION_RUN(the_reference_grid_pipelines_run_on_generic_tools_alone);
    NVISION_RUN(no_step_of_the_pipeline_needed_an_application_specific_tool);
    return 0;
}
