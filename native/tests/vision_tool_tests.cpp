// Behaviour tests for the generic image-processing tools.
//
// Every expectation is a property that holds exactly: an operation that must be the
// identity, a neighbourhood whose result is known, or a constant image that must stay
// constant. Fixtures are generated here rather than stored, so the suite runs anywhere
// and carries no asset provenance.

#include "nvision/acquisition/image.hpp"
#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/vision/builtin_tool_implementations.hpp"
#include "nvision/workflow/builtin_catalog.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/port_value.hpp"
#include "nvision/workflow/tool_execution.hpp"
#include "nvision/workflow/tool_implementation_registry.hpp"
#include "nvision/workflow/workflow_node.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Reports where an expectation failed before aborting, so a red run names the property
// that broke rather than only the exit code.
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
using nvision::core::AxisAlignedRectangle;
using nvision::core::CoordinateSpaceId;
using nvision::core::EnumOptionKey;
using nvision::core::ExecutionSessionId;
using nvision::core::ImageId;
using nvision::core::ParameterKey;
using nvision::core::PortKey;
using nvision::core::ResultCode;
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

const SemanticVersion& initial_version() {
    static const SemanticVersion version(1U, 0U, 0U);
    return version;
}

ImageView make_image(
    const std::string& identity,
    const PixelFormat format,
    const std::uint32_t width,
    const std::uint32_t height,
    std::vector<std::uint8_t> bytes) {
    const auto stride = static_cast<std::uint32_t>(
        width * nvision::acquisition::bytes_per_pixel(format));
    return ImageView(std::make_shared<const Image>(
        ImageId(identity), width, height, stride, format, std::move(bytes)));
}

ImageView constant_mono(const std::string& identity, const std::uint32_t size, const std::uint8_t value) {
    return make_image(
        identity, PixelFormat::mono8, size, size, std::vector<std::uint8_t>(size * size, value));
}

// Runs one catalog tool through the same path a workflow would use.
class Harness final {
public:
    Harness()
        : definitions_(create_builtin_registry()),
          implementations_(create_vision_implementation_registry()) {}

    [[nodiscard]] ToolExecutionOutcome run(
        const std::string& tool_id,
        const std::string& node,
        PortValueMap inputs,
        NodeConfiguration configuration = {}) const {
        const auto binding =
            implementations_.bind(ToolTypeId(tool_id), initial_version(), definitions_);
        NVISION_REQUIRE(binding.is_bound());
        const ToolExecutionContext context(
            binding.definition(),
            ExecutionSessionId("session.01"),
            WorkflowNodeId(node),
            std::move(inputs),
            ParameterBinding::resolve(*binding.definition(), configuration));
        return execute_tool(*binding.implementation(), context);
    }

private:
    ToolRegistry definitions_;
    ToolImplementationRegistry implementations_;
};

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

NodeConfiguration configure(std::vector<NodeParameterAssignment> assignments) {
    return NodeConfiguration(std::move(assignments));
}

NodeParameterAssignment integer(const std::string& key, const std::int64_t value) {
    return NodeParameterAssignment(ParameterKey(key), ParameterValue::integer(value));
}

NodeParameterAssignment number(const std::string& key, const double value) {
    return NodeParameterAssignment(ParameterKey(key), ParameterValue::floating_point(value));
}

NodeParameterAssignment option(const std::string& key, const std::string& value) {
    return NodeParameterAssignment(
        ParameterKey(key), ParameterValue::enumeration(EnumOptionKey(value)));
}

NodeParameterAssignment numbers(const std::string& key, const std::vector<double>& values) {
    std::vector<ParameterValue> items;
    items.reserve(values.size());
    for (const double value : values) {
        items.push_back(ParameterValue::floating_point(value));
    }
    return NodeParameterAssignment(ParameterKey(key), ParameterValue::array(std::move(items)));
}

const ImageView& produced(const ToolExecutionOutcome& outcome, const std::string& port) {
    NVISION_REQUIRE(outcome.is_success());
    return outcome.outputs().find(PortKey(port))->image_value();
}

ResultCode refusal(const ToolExecutionOutcome& outcome) {
    NVISION_REQUIRE(!outcome.is_success());
    NVISION_REQUIRE(outcome.outputs().empty());
    return outcome.errors().front().code();
}

bool pixels_match(const ImageView& left, const ImageView& right) {
    if (left.width() != right.width() || left.height() != right.height() ||
        left.pixel_format() != right.pixel_format()) {
        return false;
    }
    for (std::uint32_t row = 0U; row < left.height(); ++row) {
        const auto left_row = left.row(row);
        const auto right_row = right.row(row);
        if (left_row.size() != right_row.size() ||
            std::memcmp(left_row.data(), right_row.data(), left_row.size()) != 0) {
            return false;
        }
    }
    return true;
}

float float_at(const ImageView& image, const std::uint32_t x, const std::uint32_t y) {
    float value = 0.0F;
    std::memcpy(&value, image.row(y).data() + static_cast<std::size_t>(x) * 4U, sizeof(float));
    return value;
}

Transform2D identity(const CoordinateSpaceId& space) {
    return Transform2D(
        space, space, std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0});
}

void color_convert_round_trips_and_refuses_a_format_it_cannot_read() {
    const Harness harness;
    const ImageView gray =
        make_image("test.gray", PixelFormat::mono8, 2U, 2U, {10U, 20U, 30U, 40U});

    const ToolExecutionOutcome to_color = harness.run(
        "nvision.tool.processing.color-convert",
        "node.to-bgr",
        image_input(gray),
        configure({option("conversion", "gray-to-bgr")}));
    const ImageView& color = produced(to_color, "image");
    NVISION_REQUIRE(color.pixel_format() == PixelFormat::bgr8);
    // Grey expands to equal blue, green and red, so every triple repeats its source value.
    NVISION_REQUIRE(color.row(0U)[0] == 10U && color.row(0U)[1] == 10U && color.row(0U)[2] == 10U);
    NVISION_REQUIRE(color.row(1U)[3] == 40U);

    const ToolExecutionOutcome back = harness.run(
        "nvision.tool.processing.color-convert",
        "node.to-gray",
        image_input(color),
        configure({option("conversion", "bgr-to-gray")}));
    NVISION_REQUIRE(pixels_match(produced(back, "image"), gray));

    const ToolExecutionOutcome hsv = harness.run(
        "nvision.tool.processing.color-convert",
        "node.to-hsv",
        image_input(color),
        configure({option("conversion", "bgr-to-hsv")}));
    // The channels no longer mean colour primaries, and the format records that.
    NVISION_REQUIRE(produced(hsv, "image").pixel_format() == PixelFormat::hsv8);

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.color-convert",
            "node.wrong",
            image_input(gray),
            configure({option("conversion", "bgr-to-hsv")}))) ==
        ResultCode("UNSUPPORTED_PIXEL_FORMAT"));
}

void channel_extract_selects_one_plane_and_bounds_its_index() {
    const Harness harness;
    const ImageView color = make_image(
        "test.color", PixelFormat::bgr8, 2U, 1U, {1U, 2U, 3U, 4U, 5U, 6U});

    const ToolExecutionOutcome green = harness.run(
        "nvision.tool.processing.channel-extract",
        "node.green",
        image_input(color),
        configure({integer("channel", 1)}));
    const ImageView& plane = produced(green, "image");
    NVISION_REQUIRE(plane.pixel_format() == PixelFormat::mono8);
    NVISION_REQUIRE(plane.row(0U)[0] == 2U);
    NVISION_REQUIRE(plane.row(0U)[1] == 5U);

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.channel-extract",
            "node.out-of-range",
            image_input(color),
            configure({integer("channel", 3)}))) == ResultCode("CHANNEL_OUT_OF_RANGE"));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.channel-extract",
            "node.single",
            image_input(constant_mono("test.flat", 2U, 5U)),
            configure({integer("channel", 0)}))) == ResultCode("UNSUPPORTED_PIXEL_FORMAT"));
}

void resize_averages_whole_blocks_and_publishes_its_scale() {
    const Harness harness;
    // Four 2x2 blocks of a constant value, so averaging each block is exact.
    const ImageView blocks = make_image(
        "test.blocks",
        PixelFormat::mono8,
        4U,
        4U,
        {10U, 10U, 20U, 20U,
         10U, 10U, 20U, 20U,
         30U, 30U, 40U, 40U,
         30U, 30U, 40U, 40U});

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.resize",
        "node.resize",
        image_input(blocks),
        configure({integer("width", 2), integer("height", 2), option("interpolation", "area")}));

    const ImageView& small = produced(outcome, "image");
    NVISION_REQUIRE(small.width() == 2U && small.height() == 2U);
    NVISION_REQUIRE(small.row(0U)[0] == 10U && small.row(0U)[1] == 20U);
    NVISION_REQUIRE(small.row(1U)[0] == 30U && small.row(1U)[1] == 40U);

    const auto& transform = outcome.outputs().find(PortKey("transform"))->transform_value();
    NVISION_REQUIRE(transform.source_space() == small.coordinate_space());
    NVISION_REQUIRE(transform.target_space() == blocks.coordinate_space());
    NVISION_REQUIRE(transform.matrix()[0] == 2.0);
    NVISION_REQUIRE(transform.matrix()[4] == 2.0);
}

void normalize_stretches_to_the_requested_range() {
    const Harness harness;
    const ImageView image =
        make_image("test.range", PixelFormat::mono8, 3U, 1U, {10U, 20U, 30U});

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.normalize",
        "node.normalize",
        image_input(image),
        configure({number("minimum", 0.0), number("maximum", 255.0)}));
    const ImageView& stretched = produced(outcome, "image");
    NVISION_REQUIRE(stretched.row(0U)[0] == 0U);
    NVISION_REQUIRE(stretched.row(0U)[2] == 255U);

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.normalize",
            "node.bad-range",
            image_input(image),
            configure({number("minimum", 200.0), number("maximum", 100.0)}))) ==
        ResultCode("INVALID_RANGE"));
}

void clahe_leaves_a_constant_image_constant_and_needs_one_channel() {
    const Harness harness;
    const ImageView flat = constant_mono("test.flat-clahe", 8U, 128U);

    const ToolExecutionOutcome outcome =
        harness.run("nvision.tool.processing.clahe", "node.clahe", image_input(flat));
    const ImageView& equalized = produced(outcome, "image");
    NVISION_REQUIRE(equalized.width() == 8U && equalized.height() == 8U);
    NVISION_REQUIRE(equalized.pixel_format() == PixelFormat::mono8);
    // An image with one occupied histogram bin has no local contrast to redistribute, so
    // it must stay uniform. Equalisation is free to move where that single level sits, so
    // the contract is uniformity rather than an unchanged value.
    const std::uint8_t level = equalized.row(0U)[0];
    for (std::uint32_t row = 0U; row < equalized.height(); ++row) {
        for (std::uint32_t column = 0U; column < equalized.width(); ++column) {
            NVISION_REQUIRE(equalized.row(row)[column] == level);
        }
    }

    const ImageView color =
        make_image("test.clahe-color", PixelFormat::bgr8, 1U, 1U, {1U, 2U, 3U});
    NVISION_REQUIRE(
        refusal(harness.run("nvision.tool.processing.clahe", "node.clahe-color", image_input(color))) ==
        ResultCode("UNSUPPORTED_PIXEL_FORMAT"));
}

void blur_preserves_a_constant_image_and_rejects_an_even_kernel() {
    const Harness harness;
    const ImageView flat = constant_mono("test.flat-blur", 5U, 77U);

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.gaussian-blur",
        "node.blur",
        image_input(flat),
        configure({integer("kernel-size", 3), number("sigma", 0.0)}));
    // Any weighted average of one repeated value is that value.
    NVISION_REQUIRE(pixels_match(produced(outcome, "image"), flat));

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.gaussian-blur",
            "node.even",
            image_input(flat),
            configure({integer("kernel-size", 4)}))) == ResultCode("INVALID_KERNEL_SIZE"));
}

void sharpening_by_zero_is_the_identity() {
    const Harness harness;
    const ImageView image = make_image(
        "test.sharpen", PixelFormat::mono8, 3U, 3U,
        {0U, 40U, 0U, 40U, 200U, 40U, 0U, 40U, 0U});

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.sharpen",
        "node.sharpen",
        image_input(image),
        configure({number("strength", 0.0)}));
    NVISION_REQUIRE(pixels_match(produced(outcome, "image"), image));

    const ToolExecutionOutcome amplified = harness.run(
        "nvision.tool.processing.sharpen",
        "node.sharpen-strong",
        image_input(image),
        configure({number("strength", 1.0)}));
    // Unsharp masking pushes a bright centre further above its blurred neighbourhood.
    NVISION_REQUIRE(produced(amplified, "image").row(1U)[1] == 255U);
}

void adaptive_threshold_excludes_a_flat_neighbourhood() {
    const Harness harness;
    const ImageView flat = constant_mono("test.flat-adaptive", 9U, 100U);

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.adaptive-threshold",
        "node.adaptive",
        image_input(flat),
        configure({integer("block-size", 3), number("offset", 0.0), option("method", "mean")}));
    const ImageView& mask = produced(outcome, "mask");
    NVISION_REQUIRE(mask.pixel_format() == PixelFormat::mono8);
    // No pixel is above its own neighbourhood mean, so nothing is selected.
    for (std::uint32_t row = 0U; row < mask.height(); ++row) {
        for (std::uint32_t column = 0U; column < mask.width(); ++column) {
            NVISION_REQUIRE(mask.row(row)[column] == 0U);
        }
    }

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.adaptive-threshold",
            "node.even-block",
            image_input(flat),
            configure({integer("block-size", 4)}))) == ResultCode("INVALID_BLOCK_SIZE"));
}

void color_range_selects_only_pixels_inside_every_bound() {
    const Harness harness;
    const ImageView color = make_image(
        "test.range-color", PixelFormat::bgr8, 2U, 1U, {10U, 10U, 10U, 200U, 200U, 200U});

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.color-range",
        "node.range",
        image_input(color),
        configure({numbers("lower", {0.0, 0.0, 0.0}), numbers("upper", {50.0, 50.0, 50.0})}));
    const ImageView& mask = produced(outcome, "mask");
    NVISION_REQUIRE(mask.row(0U)[0] == 255U);
    NVISION_REQUIRE(mask.row(0U)[1] == 0U);

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.color-range",
            "node.wrong-bounds",
            image_input(color),
            configure({numbers("lower", {0.0}), numbers("upper", {50.0})}))) ==
        ResultCode("BOUND_CHANNEL_MISMATCH"));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.color-range",
            "node.inverted",
            image_input(color),
            configure({numbers("lower", {90.0, 0.0, 0.0}), numbers("upper", {50.0, 50.0, 50.0})}))) ==
        ResultCode("INVALID_RANGE"));
}

void morphology_grows_and_shrinks_a_single_selected_pixel() {
    const Harness harness;
    std::vector<std::uint8_t> pixels(25U, 0U);
    pixels[12U] = 255U;  // the centre of a 5x5 mask
    const ImageView dot = make_image("test.dot", PixelFormat::mono8, 5U, 5U, pixels);

    const ToolExecutionOutcome dilated = harness.run(
        "nvision.tool.processing.morphology",
        "node.dilate",
        mask_input(dot),
        configure({option("operation", "dilate"), integer("kernel-size", 3), integer("iterations", 1)}));
    const ImageView& grown = produced(dilated, "mask");
    // A 3x3 rectangular element turns one selected pixel into its whole neighbourhood.
    NVISION_REQUIRE(grown.row(1U)[1] == 255U);
    NVISION_REQUIRE(grown.row(3U)[3] == 255U);
    NVISION_REQUIRE(grown.row(0U)[0] == 0U);

    const ToolExecutionOutcome eroded = harness.run(
        "nvision.tool.processing.morphology",
        "node.erode",
        mask_input(dot),
        configure({option("operation", "erode"), integer("kernel-size", 3), integer("iterations", 1)}));
    const ImageView& shrunk = produced(eroded, "mask");
    NVISION_REQUIRE(shrunk.row(2U)[2] == 0U);
}

void the_distance_transform_reports_pixels_not_intensity() {
    const Harness harness;
    // A 5x5 mask selected everywhere except its border.
    std::vector<std::uint8_t> pixels(25U, 0U);
    for (std::uint32_t row = 1U; row < 4U; ++row) {
        for (std::uint32_t column = 1U; column < 4U; ++column) {
            pixels[row * 5U + column] = 255U;
        }
    }
    const ImageView mask = make_image("test.block", PixelFormat::mono8, 5U, 5U, pixels);

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.processing.distance-transform",
        "node.distance",
        mask_input(mask),
        configure({option("distance-type", "chessboard")}));
    const ImageView& distances = produced(outcome, "image");
    NVISION_REQUIRE(distances.pixel_format() == PixelFormat::mono_float32);
    // The centre is two chessboard steps from the nearest excluded pixel, and a border
    // pixel is zero steps because it is excluded itself.
    NVISION_REQUIRE(float_at(distances, 2U, 2U) == 2.0F);
    NVISION_REQUIRE(float_at(distances, 1U, 1U) == 1.0F);
    NVISION_REQUIRE(float_at(distances, 0U, 0U) == 0.0F);
}

void warping_by_an_identity_mapping_returns_the_same_pixels() {
    const Harness harness;
    const ImageView image = make_image(
        "test.warp", PixelFormat::mono8, 3U, 3U,
        {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U});

    PortValueMap perspective_inputs = image_input(image);
    perspective_inputs.set(
        PortKey("homography"), PortValue::homography(identity(image.coordinate_space())));
    const ToolExecutionOutcome warped = harness.run(
        "nvision.tool.processing.perspective-warp",
        "node.warp",
        std::move(perspective_inputs),
        configure({integer("width", 0), integer("height", 0)}));
    NVISION_REQUIRE(pixels_match(produced(warped, "image"), image));

    PortValueMap affine_inputs = image_input(image);
    affine_inputs.set(PortKey("transform"), PortValue::transform(identity(image.coordinate_space())));
    const ToolExecutionOutcome affine = harness.run(
        "nvision.tool.processing.affine-transform", "node.affine", std::move(affine_inputs));
    NVISION_REQUIRE(pixels_match(produced(affine, "image"), image));
}

void warping_refuses_a_mapping_it_cannot_honour() {
    const Harness harness;
    const ImageView image = constant_mono("test.warp-refusal", 3U, 5U);
    const CoordinateSpaceId elsewhere("nvision.image.other.image-pixel");

    PortValueMap wrong_space = image_input(image);
    wrong_space.set(PortKey("homography"), PortValue::homography(identity(elsewhere)));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.perspective-warp",
            "node.warp-space",
            std::move(wrong_space),
            configure({integer("width", 0), integer("height", 0)}))) ==
        ResultCode("COORDINATE_SPACE_MISMATCH"));

    // A singular mapping never reaches a tool: Transform2D refuses to exist without an
    // inverse, so the warp does not need a branch for one.
    bool rejected = false;
    try {
        static_cast<void>(Transform2D(
            image.coordinate_space(),
            image.coordinate_space(),
            std::array<double, 9>{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    NVISION_REQUIRE(rejected);

    // A projective matrix on the affine port would quietly lose its perspective row.
    PortValueMap projective = image_input(image);
    projective.set(
        PortKey("transform"),
        PortValue::transform(Transform2D(
            image.coordinate_space(),
            image.coordinate_space(),
            std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.001, 0.0, 1.0})));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.processing.affine-transform", "node.affine-projective",
            std::move(projective))) == ResultCode("TRANSFORM_NOT_AFFINE"));
}

void the_image_processing_group_is_fully_implemented() {
    const ToolRegistry definitions = create_builtin_registry();
    const ToolImplementationRegistry implementations = create_vision_implementation_registry();

    std::size_t processing_definitions = 0U;
    std::size_t processing_implemented = 0U;
    for (const auto& definition : definitions.definitions()) {
        if (definition->id().value().rfind("nvision.tool.processing.", 0U) != 0U) {
            continue;
        }
        ++processing_definitions;
        if (implementations.contains(definition->id(), definition->version())) {
            ++processing_implemented;
        }
    }

    NVISION_REQUIRE(processing_definitions == 15U);
    NVISION_REQUIRE(processing_implemented == processing_definitions);
}

}  // namespace

namespace {

// Names the test that failed and reports an exception that escaped it, so a red run is
// diagnosable from the CTest output alone.
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
    NVISION_RUN(color_convert_round_trips_and_refuses_a_format_it_cannot_read);
    NVISION_RUN(channel_extract_selects_one_plane_and_bounds_its_index);
    NVISION_RUN(resize_averages_whole_blocks_and_publishes_its_scale);
    NVISION_RUN(normalize_stretches_to_the_requested_range);
    NVISION_RUN(clahe_leaves_a_constant_image_constant_and_needs_one_channel);
    NVISION_RUN(blur_preserves_a_constant_image_and_rejects_an_even_kernel);
    NVISION_RUN(sharpening_by_zero_is_the_identity);
    NVISION_RUN(adaptive_threshold_excludes_a_flat_neighbourhood);
    NVISION_RUN(color_range_selects_only_pixels_inside_every_bound);
    NVISION_RUN(morphology_grows_and_shrinks_a_single_selected_pixel);
    NVISION_RUN(the_distance_transform_reports_pixels_not_intensity);
    NVISION_RUN(warping_by_an_identity_mapping_returns_the_same_pixels);
    NVISION_RUN(warping_refuses_a_mapping_it_cannot_honour);
    NVISION_RUN(the_image_processing_group_is_fully_implemented);
    return 0;
}
