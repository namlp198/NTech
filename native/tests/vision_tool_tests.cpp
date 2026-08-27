// Behaviour tests for the generic image-processing tools.
//
// Every expectation is a property that holds exactly: an operation that must be the
// identity, a neighbourhood whose result is known, or a constant image that must stay
// constant. Fixtures are generated here rather than stored, so the suite runs anywhere
// and carries no asset provenance.

#include "nvision/acquisition/image.hpp"
#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/detection.hpp"
#include "nvision/core/feature.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/core/measurement.hpp"
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
using nvision::core::Contour;
using nvision::core::CoordinateSpaceId;
using nvision::core::EnumOptionKey;
using nvision::core::ExecutionSessionId;
using nvision::core::Feature;
using nvision::core::FeatureMatch;
using nvision::core::FeatureSet;
using nvision::core::ImageId;
using nvision::core::MatchSet;
using nvision::core::MeasurementUnit;
using nvision::core::ParameterKey;
using nvision::core::Point2D;
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

// An 8x8 mask holding two separated 2x2 selected regions, so counts, areas and bounds are
// all known without depending on how an algorithm orders its results.
ImageView two_blocks(const std::string& identity) {
    std::vector<std::uint8_t> pixels(64U, 0U);
    for (std::uint32_t row = 1U; row < 3U; ++row) {
        for (std::uint32_t column = 1U; column < 3U; ++column) {
            pixels[row * 8U + column] = 255U;
        }
    }
    for (std::uint32_t row = 5U; row < 7U; ++row) {
        for (std::uint32_t column = 5U; column < 7U; ++column) {
            pixels[row * 8U + column] = 255U;
        }
    }
    return make_image(identity, PixelFormat::mono8, 8U, 8U, std::move(pixels));
}

// A deterministic textured image. Feature detection and ECC both need local structure,
// and a fixed generator keeps the fixture reproducible without storing an asset.
ImageView textured(const std::string& identity, const std::uint32_t shift) {
    std::vector<std::uint8_t> pixels(64U * 64U);
    for (std::uint32_t row = 0U; row < 64U; ++row) {
        for (std::uint32_t column = 0U; column < 64U; ++column) {
            const std::uint32_t x = (column + shift) % 64U;
            const std::uint32_t y = (row + shift) % 64U;
            const std::uint32_t value = (x * 7U + y * 13U + (x * y) % 29U) * 37U;
            pixels[row * 64U + column] = static_cast<std::uint8_t>(value % 256U);
        }
    }
    return make_image(identity, PixelFormat::mono8, 64U, 64U, std::move(pixels));
}

const nvision::core::DetectionList& detections(
    const ToolExecutionOutcome& outcome,
    const std::string& port) {
    NVISION_REQUIRE(outcome.is_success());
    return outcome.outputs().find(PortKey(port))->detection_list_value();
}

const nvision::core::MeasurementList& measurements(
    const ToolExecutionOutcome& outcome,
    const std::string& port) {
    NVISION_REQUIRE(outcome.is_success());
    return outcome.outputs().find(PortKey(port))->measurement_list_value();
}

Transform2D translation(
    const CoordinateSpaceId& from,
    const CoordinateSpaceId& to,
    const double x,
    const double y) {
    return Transform2D(from, to, std::array<double, 9>{1.0, 0.0, x, 0.0, 1.0, y, 0.0, 0.0, 1.0});
}

void connected_components_reports_each_region_with_its_own_geometry() {
    const Harness harness;
    const ImageView mask = two_blocks("test.components");

    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.geometry.connected-components", "node.components", mask_input(mask));
    NVISION_REQUIRE(outcome.outputs().find(PortKey("count"))->integer_value() == 2);

    const auto& components = detections(outcome, "components");
    NVISION_REQUIRE(components.size() == 2U);
    for (const auto& component : components) {
        NVISION_REQUIRE(component.area() == 4.0);
        NVISION_REQUIRE(component.bounds().width() == 2.0);
        NVISION_REQUIRE(component.bounds().height() == 2.0);
        // A region is reported in the space of the mask it was found in.
        NVISION_REQUIRE(component.coordinate_space() == mask.coordinate_space());
    }

    // A minimum area larger than either region leaves nothing to report.
    const ToolExecutionOutcome filtered = harness.run(
        "nvision.tool.geometry.connected-components",
        "node.components-filtered",
        mask_input(mask),
        configure({number("minimum-area", 5.0)}));
    NVISION_REQUIRE(filtered.outputs().find(PortKey("count"))->integer_value() == 0);

    const ImageView color = make_image("test.cc-color", PixelFormat::bgr8, 1U, 1U, {1U, 2U, 3U});
    PortValueMap wrong_format;
    wrong_format.set(PortKey("mask"), PortValue::mask(color));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.connected-components",
            "node.cc-format",
            std::move(wrong_format))) == ResultCode("UNSUPPORTED_PIXEL_FORMAT"));
}

void contour_extraction_and_blob_analysis_agree_on_what_is_there() {
    const Harness harness;
    const ImageView mask = two_blocks("test.contours");

    const ToolExecutionOutcome extracted = harness.run(
        "nvision.tool.geometry.contour-extraction", "node.contours", mask_input(mask));
    NVISION_REQUIRE(extracted.is_success());
    const auto& contours =
        extracted.outputs().find(PortKey("contours"))->contour_list_value();
    NVISION_REQUIRE(contours.size() == 2U);
    for (const Contour& contour : contours) {
        NVISION_REQUIRE(contour.coordinate_space() == mask.coordinate_space());
        NVISION_REQUIRE(!contour.points().empty());
        // External retrieval reports no enclosure, so no contour claims a parent.
        NVISION_REQUIRE(!contour.parent().has_value());
    }

    const ToolExecutionOutcome analysed = harness.run(
        "nvision.tool.geometry.blob-analysis", "node.blobs", mask_input(mask));
    NVISION_REQUIRE(detections(analysed, "blobs").size() == 2U);
    // One area and one perimeter per blob, each naming the space it was measured in.
    const auto& blob_measurements = measurements(analysed, "measurements");
    NVISION_REQUIRE(blob_measurements.size() == 4U);
    NVISION_REQUIRE(blob_measurements[0].unit() == MeasurementUnit::square_pixel);
    NVISION_REQUIRE(blob_measurements[1].unit() == MeasurementUnit::pixel);
    NVISION_REQUIRE(blob_measurements[0].coordinate_space() == mask.coordinate_space());

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.blob-analysis",
            "node.blobs-range",
            mask_input(mask),
            configure({number("minimum-area", 10.0), number("maximum-area", 5.0)}))) ==
        ResultCode("INVALID_RANGE"));
}

void the_minimum_enclosing_circle_covers_its_contour() {
    const Harness harness;
    const CoordinateSpaceId space("nvision.image.node.square.image-pixel");
    // The corners of a 4 by 4 square, whose enclosing circle is centred on the middle with
    // a radius of half the diagonal.
    const Contour square({
        Point2D(space, 0.0, 0.0),
        Point2D(space, 4.0, 0.0),
        Point2D(space, 4.0, 4.0),
        Point2D(space, 0.0, 4.0),
    });

    PortValueMap inputs;
    inputs.set(PortKey("contours"), PortValue::contour_list({square}));
    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.geometry.minimum-enclosing-circle", "node.circles", std::move(inputs));
    NVISION_REQUIRE(outcome.is_success());

    const auto& centers = outcome.outputs().find(PortKey("centers"))->point_list_value();
    NVISION_REQUIRE(centers.size() == 1U);
    NVISION_REQUIRE(std::abs(centers[0].x() - 2.0) < 0.01);
    NVISION_REQUIRE(std::abs(centers[0].y() - 2.0) < 0.01);
    NVISION_REQUIRE(centers[0].coordinate_space() == space);

    const auto& radii = measurements(outcome, "radii");
    NVISION_REQUIRE(radii.size() == 1U);
    NVISION_REQUIRE(radii[0].unit() == MeasurementUnit::pixel);
    NVISION_REQUIRE(std::abs(radii[0].value() - std::sqrt(8.0)) < 0.01);
}

void line_fit_recovers_a_line_the_points_lie_on() {
    const Harness harness;
    const CoordinateSpaceId space("nvision.image.node.line.image-pixel");
    std::vector<Point2D> collinear;
    for (int step = 0; step < 5; ++step) {
        // y = 2x + 1, so a correct fit has no residual left over.
        collinear.emplace_back(space, static_cast<double>(step), 2.0 * step + 1.0);
    }

    PortValueMap inputs;
    inputs.set(PortKey("points"), PortValue::point_list(collinear));
    const ToolExecutionOutcome outcome =
        harness.run("nvision.tool.geometry.line-fit", "node.line", std::move(inputs));
    NVISION_REQUIRE(outcome.is_success());
    NVISION_REQUIRE(
        outcome.outputs().find(PortKey("residual"))->floating_point_value() < 1.0e-6);

    const auto& line = outcome.outputs().find(PortKey("line"))->line_value();
    NVISION_REQUIRE(line.coordinate_space() == space);
    // The reported segment spans the fitted points rather than an arbitrary length.
    NVISION_REQUIRE(std::abs(std::hypot(
                        line.end().x() - line.start().x(),
                        line.end().y() - line.start().y()) -
                        std::sqrt(80.0)) < 0.01);

    PortValueMap single;
    single.set(PortKey("points"), PortValue::point_list({Point2D(space, 1.0, 1.0)}));
    NVISION_REQUIRE(
        refusal(harness.run("nvision.tool.geometry.line-fit", "node.line-one", std::move(single))) ==
        ResultCode("NOT_ENOUGH_POINTS"));

    PortValueMap repeated;
    repeated.set(
        PortKey("points"),
        PortValue::point_list({Point2D(space, 1.0, 1.0), Point2D(space, 1.0, 1.0)}));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.line-fit", "node.line-same", std::move(repeated))) ==
        ResultCode("DEGENERATE_POINTS"));
}

void feature_matching_relates_two_rasters_and_refuses_to_match_one_with_itself() {
    const Harness harness;
    const ImageView live = textured("test.live", 0U);
    const ImageView golden = textured("test.golden", 3U);

    PortValueMap inputs;
    inputs.set(PortKey("image"), PortValue::image(live));
    inputs.set(PortKey("reference"), PortValue::image(golden));
    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.geometry.feature-detect-and-match",
        "node.match",
        std::move(inputs),
        configure({option("backend", "orb")}));
    NVISION_REQUIRE(outcome.is_success());

    const auto& matches = outcome.outputs().find(PortKey("matches"))->match_set_value();
    NVISION_REQUIRE(matches.query().coordinate_space() == live.coordinate_space());
    NVISION_REQUIRE(matches.reference().coordinate_space() == golden.coordinate_space());
    // Whatever the detector found, every reported correspondence must resolve to a real
    // point pair through the sets the match set carries.
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        NVISION_REQUIRE(matches.query_point(index).coordinate_space() == live.coordinate_space());
        NVISION_REQUIRE(
            matches.reference_point(index).coordinate_space() == golden.coordinate_space());
    }
    const auto& features = outcome.outputs().find(PortKey("features"))->feature_set_value();
    NVISION_REQUIRE(features.coordinate_space() == live.coordinate_space());

    PortValueMap same;
    same.set(PortKey("image"), PortValue::image(live));
    same.set(PortKey("reference"), PortValue::image(live));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.feature-detect-and-match",
            "node.match-self",
            std::move(same))) == ResultCode("SAME_COORDINATE_SPACE"));
}

void a_homography_is_estimated_from_correspondences_and_then_judged() {
    const Harness harness;
    const CoordinateSpaceId query_space("nvision.image.node.live.image-pixel");
    const CoordinateSpaceId reference_space("nvision.image.node.golden.image-pixel");

    // Four correspondences describing a pure shift of five across and seven down, built by
    // hand so the expectation does not depend on what a detector happens to find.
    std::vector<Feature> query_features;
    std::vector<Feature> reference_features;
    std::vector<FeatureMatch> pairs;
    const double positions[4][2] = {{0.0, 0.0}, {20.0, 0.0}, {20.0, 30.0}, {0.0, 30.0}};
    for (std::size_t index = 0U; index < 4U; ++index) {
        query_features.emplace_back(
            Point2D(query_space, positions[index][0], positions[index][1]), 3.0, 0.0, 1.0);
        reference_features.emplace_back(
            Point2D(reference_space, positions[index][0] + 5.0, positions[index][1] + 7.0),
            3.0,
            0.0,
            1.0);
        pairs.emplace_back(index, index, 0.1);
    }

    PortValueMap inputs;
    inputs.set(
        PortKey("matches"),
        PortValue::match_set(MatchSet(
            FeatureSet(query_space, query_features),
            FeatureSet(reference_space, reference_features),
            pairs)));
    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.geometry.homography-estimate", "node.homography", std::move(inputs));
    NVISION_REQUIRE(outcome.is_success());
    NVISION_REQUIRE(
        outcome.outputs().find(PortKey("inlier-ratio"))->floating_point_value() == 1.0);

    const auto& homography = outcome.outputs().find(PortKey("homography"))->transform_value();
    NVISION_REQUIRE(homography.source_space() == query_space);
    NVISION_REQUIRE(homography.target_space() == reference_space);
    const Point2D mapped = homography.apply(Point2D(query_space, 10.0, 10.0));
    NVISION_REQUIRE(std::abs(mapped.x() - 15.0) < 0.01);
    NVISION_REQUIRE(std::abs(mapped.y() - 17.0) < 0.01);

    PortValueMap too_few;
    too_few.set(
        PortKey("matches"),
        PortValue::match_set(MatchSet(
            FeatureSet(query_space, query_features),
            FeatureSet(reference_space, reference_features),
            {pairs[0], pairs[1]})));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.homography-estimate",
            "node.homography-few",
            std::move(too_few))) == ResultCode("NOT_ENOUGH_MATCHES"));

    PortValueMap sound;
    sound.set(PortKey("homography"), PortValue::homography(homography));
    const ToolExecutionOutcome judged = harness.run(
        "nvision.tool.geometry.homography-validate", "node.validate", std::move(sound));
    NVISION_REQUIRE(judged.outputs().find(PortKey("valid"))->boolean_value());
    const auto& judgement = measurements(judged, "measurements");
    NVISION_REQUIRE(judgement.size() == 2U);
    NVISION_REQUIRE(std::abs(judgement[0].value() - 1.0) < 0.01);
    NVISION_REQUIRE(judgement[1].value() < 0.01);

    // A mapping that stretches one axis twice as far as the other is reported as such.
    PortValueMap stretched;
    stretched.set(
        PortKey("homography"),
        PortValue::homography(Transform2D(
            query_space,
            reference_space,
            std::array<double, 9>{2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0})));
    const ToolExecutionOutcome rejected = harness.run(
        "nvision.tool.geometry.homography-validate",
        "node.validate-stretch",
        std::move(stretched),
        configure({number("maximum-scale-deviation", 0.1)}));
    NVISION_REQUIRE(!rejected.outputs().find(PortKey("valid"))->boolean_value());
    NVISION_REQUIRE(std::abs(measurements(rejected, "measurements")[1].value() - 0.5) < 0.01);
}

void ecc_refinement_starts_from_a_transform_between_the_two_spaces_it_was_given() {
    const Harness harness;
    const ImageView live = textured("test.ecc-live", 0U);
    const ImageView golden = textured("test.ecc-golden", 0U);

    PortValueMap inputs;
    inputs.set(PortKey("image"), PortValue::image(live));
    inputs.set(PortKey("reference"), PortValue::image(golden));
    inputs.set(
        PortKey("transform"),
        PortValue::transform(
            translation(live.coordinate_space(), golden.coordinate_space(), 0.0, 0.0)));
    const ToolExecutionOutcome outcome =
        harness.run("nvision.tool.geometry.ecc-refine", "node.ecc", std::move(inputs));
    NVISION_REQUIRE(outcome.is_success());
    // The two rasters hold identical pixels, so an identity start is already the answer.
    NVISION_REQUIRE(
        outcome.outputs().find(PortKey("correlation"))->floating_point_value() > 0.99);
    const auto& refined = outcome.outputs().find(PortKey("transform"))->transform_value();
    NVISION_REQUIRE(refined.source_space() == live.coordinate_space());
    NVISION_REQUIRE(refined.target_space() == golden.coordinate_space());

    PortValueMap wrong_direction;
    wrong_direction.set(PortKey("image"), PortValue::image(live));
    wrong_direction.set(PortKey("reference"), PortValue::image(golden));
    wrong_direction.set(
        PortKey("transform"),
        PortValue::transform(
            translation(golden.coordinate_space(), live.coordinate_space(), 0.0, 0.0)));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.ecc-refine",
            "node.ecc-reversed",
            std::move(wrong_direction))) == ResultCode("COORDINATE_SPACE_MISMATCH"));
}

void points_are_transformed_into_the_space_the_transform_names() {
    const Harness harness;
    const CoordinateSpaceId from("nvision.image.node.detected.image-pixel");
    const CoordinateSpaceId to("nvision.image.node.model.image-pixel");

    PortValueMap inputs;
    inputs.set(
        PortKey("points"),
        PortValue::point_list({Point2D(from, 1.0, 2.0), Point2D(from, 3.0, 4.0)}));
    inputs.set(PortKey("transform"), PortValue::transform(translation(from, to, 10.0, 20.0)));
    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.geometry.point-transform", "node.transform", std::move(inputs));
    NVISION_REQUIRE(outcome.is_success());

    const auto& moved = outcome.outputs().find(PortKey("points"))->point_list_value();
    NVISION_REQUIRE(moved.size() == 2U);
    NVISION_REQUIRE(moved[0].x() == 11.0 && moved[0].y() == 22.0);
    NVISION_REQUIRE(moved[1].coordinate_space() == to);

    PortValueMap mismatched;
    mismatched.set(PortKey("points"), PortValue::point_list({Point2D(to, 1.0, 2.0)}));
    mismatched.set(PortKey("transform"), PortValue::transform(translation(from, to, 1.0, 1.0)));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.point-transform",
            "node.transform-mismatch",
            std::move(mismatched))) == ResultCode("COORDINATE_SPACE_MISMATCH"));
}

void point_assignment_pairs_each_expected_position_at_most_once() {
    const Harness harness;
    const CoordinateSpaceId space("nvision.image.node.grid.image-pixel");

    PortValueMap inputs;
    // Two detected points sit close to the first two expected positions; the third
    // detection is far from every one of them.
    inputs.set(
        PortKey("points"),
        PortValue::point_list({
            Point2D(space, 1.0, 1.0),
            Point2D(space, 11.0, 1.0),
            Point2D(space, 500.0, 500.0),
        }));
    inputs.set(
        PortKey("reference"),
        PortValue::point_list({
            Point2D(space, 0.0, 0.0),
            Point2D(space, 10.0, 0.0),
            Point2D(space, 20.0, 0.0),
        }));
    const ToolExecutionOutcome outcome = harness.run(
        "nvision.tool.geometry.point-assignment",
        "node.assign",
        std::move(inputs),
        configure({number("maximum-distance", 3.0)}));
    NVISION_REQUIRE(outcome.is_success());

    const auto& assignments = measurements(outcome, "assignments");
    NVISION_REQUIRE(assignments.size() == 2U);
    NVISION_REQUIRE(assignments[0].unit() == MeasurementUnit::pixel);
    NVISION_REQUIRE(std::abs(assignments[0].value() - std::sqrt(2.0)) < 0.01);

    // The far detection was never claimed, so it is reported as unmatched.
    const auto& unmatched = outcome.outputs().find(PortKey("unmatched"))->point_list_value();
    NVISION_REQUIRE(unmatched.size() == 1U);
    NVISION_REQUIRE(unmatched[0].x() == 500.0);

    PortValueMap crossed;
    crossed.set(PortKey("points"), PortValue::point_list({Point2D(space, 0.0, 0.0)}));
    crossed.set(
        PortKey("reference"),
        PortValue::point_list(
            {Point2D(CoordinateSpaceId("nvision.image.node.other.image-pixel"), 0.0, 0.0)}));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.geometry.point-assignment",
            "node.assign-crossed",
            std::move(crossed))) == ResultCode("COORDINATE_SPACE_MISMATCH"));
}

const nvision::core::Measurement& measurement(
    const ToolExecutionOutcome& outcome,
    const std::string& port) {
    NVISION_REQUIRE(outcome.is_success());
    return outcome.outputs().find(PortKey(port))->measurement_value();
}

void counting_reports_both_how_many_and_what_share() {
    const Harness harness;
    const ImageView mask = two_blocks("test.counting");

    // Two 2 by 2 regions inside an 8 by 8 mask: eight selected pixels out of sixty-four.
    const ToolExecutionOutcome counted =
        harness.run("nvision.tool.measurement.pixel-count", "node.pixels", mask_input(mask));
    NVISION_REQUIRE(counted.outputs().find(PortKey("count"))->integer_value() == 8);
    NVISION_REQUIRE(counted.outputs().find(PortKey("ratio"))->floating_point_value() == 0.125);

    const CoordinateSpaceId space("nvision.image.node.objects.image-pixel");
    nvision::core::DetectionList objects;
    objects.emplace_back(
        AxisAlignedRectangle(space, 0.0, 0.0, 2.0, 2.0), Point2D(space, 1.0, 1.0), 4.0);
    objects.emplace_back(
        AxisAlignedRectangle(space, 4.0, 4.0, 2.0, 2.0), Point2D(space, 5.0, 5.0), 4.0);
    PortValueMap detected;
    detected.set(PortKey("objects"), PortValue::detection_list(std::move(objects)));
    const ToolExecutionOutcome objects_counted =
        harness.run("nvision.tool.measurement.object-count", "node.objects", std::move(detected));
    NVISION_REQUIRE(objects_counted.outputs().find(PortKey("count"))->integer_value() == 2);
}

void distance_and_radius_are_reported_in_the_space_they_were_measured_in() {
    const Harness harness;
    const CoordinateSpaceId space("nvision.image.node.measured.image-pixel");

    PortValueMap separation;
    separation.set(PortKey("from"), PortValue::point(Point2D(space, 0.0, 0.0)));
    separation.set(PortKey("to"), PortValue::point(Point2D(space, 3.0, 4.0)));
    const ToolExecutionOutcome measured =
        harness.run("nvision.tool.measurement.distance", "node.distance", std::move(separation));
    const auto& gap = measurement(measured, "measurement");
    NVISION_REQUIRE(gap.value() == 5.0);
    NVISION_REQUIRE(gap.unit() == MeasurementUnit::pixel);
    NVISION_REQUIRE(gap.coordinate_space() == space);

    // A separation between two spaces is not a length, so it is refused rather than
    // reported as a number that means nothing.
    PortValueMap crossed;
    crossed.set(PortKey("from"), PortValue::point(Point2D(space, 0.0, 0.0)));
    crossed.set(
        PortKey("to"),
        PortValue::point(
            Point2D(CoordinateSpaceId("nvision.image.node.other.image-pixel"), 3.0, 4.0)));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.measurement.distance", "node.distance-crossed", std::move(crossed))) ==
        ResultCode("COORDINATE_SPACE_MISMATCH"));

    PortValueMap round;
    round.set(
        PortKey("circle"),
        PortValue::circle(nvision::core::Circle(Point2D(space, 10.0, 10.0), 2.5)));
    const ToolExecutionOutcome sized =
        harness.run("nvision.tool.measurement.radius-diameter", "node.radius", std::move(round));
    const auto& sizes = measurements(sized, "measurements");
    NVISION_REQUIRE(sizes.size() == 2U);
    NVISION_REQUIRE(sizes[0].value() == 2.5);
    NVISION_REQUIRE(sizes[1].value() == 5.0);
    NVISION_REQUIRE(sizes[1].coordinate_space() == space);
}

void a_histogram_places_every_pixel_in_exactly_one_bin() {
    const Harness harness;
    const ImageView flat = constant_mono("test.histogram", 4U, 7U);

    const ToolExecutionOutcome outcome =
        harness.run("nvision.tool.measurement.histogram", "node.histogram", image_input(flat));
    const auto& histogram = measurements(outcome, "histogram");
    NVISION_REQUIRE(histogram.size() == 256U);

    // Sixteen pixels all holding the value seven land in bin seven and nowhere else.
    double total = 0.0;
    for (std::size_t bin = 0U; bin < histogram.size(); ++bin) {
        NVISION_REQUIRE(histogram[bin].unit() == MeasurementUnit::count);
        // A count is not a length, so it names no coordinate space.
        NVISION_REQUIRE(!histogram[bin].coordinate_space().has_value());
        NVISION_REQUIRE(histogram[bin].value() == (bin == 7U ? 16.0 : 0.0));
        total += histogram[bin].value();
    }
    NVISION_REQUIRE(total == 16.0);

    const ImageView color =
        make_image("test.hist-color", PixelFormat::bgr8, 1U, 1U, {1U, 2U, 3U});
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.measurement.histogram", "node.hist-color", image_input(color))) ==
        ResultCode("UNSUPPORTED_PIXEL_FORMAT"));
}

void quality_figures_are_zero_for_an_image_with_no_structure() {
    const Harness harness;
    const ImageView flat = constant_mono("test.quality-flat", 16U, 120U);
    const ImageView structured = textured("test.quality-structured", 0U);

    // A constant image has no second derivative, no spread around its mean and no edges,
    // so all three figures are exactly zero.
    NVISION_REQUIRE(
        measurement(
            harness.run("nvision.tool.measurement.sharpness", "node.sharp-flat", image_input(flat)),
            "measurement")
            .value() == 0.0);
    NVISION_REQUIRE(
        measurement(
            harness.run(
                "nvision.tool.measurement.contrast", "node.contrast-flat", image_input(flat)),
            "measurement")
            .value() == 0.0);
    const auto& density = measurement(
        harness.run("nvision.tool.measurement.edge-density", "node.edges-flat", image_input(flat)),
        "measurement");
    NVISION_REQUIRE(density.value() == 0.0);
    NVISION_REQUIRE(density.unit() == MeasurementUnit::ratio);

    // A textured image has all three, and a quality figure is a pure number rather than a
    // length, so none of them names a coordinate space.
    const auto& sharpness = measurement(
        harness.run(
            "nvision.tool.measurement.sharpness", "node.sharp-real", image_input(structured)),
        "measurement");
    NVISION_REQUIRE(sharpness.value() > 0.0);
    NVISION_REQUIRE(sharpness.unit() == MeasurementUnit::none);
    NVISION_REQUIRE(!sharpness.coordinate_space().has_value());
    NVISION_REQUIRE(
        measurement(
            harness.run(
                "nvision.tool.measurement.contrast", "node.contrast-real", image_input(structured)),
            "measurement")
            .value() > 0.0);

    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.measurement.edge-density",
            "node.edges-range",
            image_input(flat),
            configure({number("low-threshold", 200.0), number("high-threshold", 50.0)}))) ==
        ResultCode("INVALID_RANGE"));
}

void comparing_a_raster_with_itself_scores_perfectly() {
    const Harness harness;
    const ImageView structured = textured("test.compare", 0U);
    const ImageView same_pixels = textured("test.compare-copy", 0U);

    PortValueMap ncc_inputs;
    ncc_inputs.set(PortKey("image"), PortValue::image(structured));
    ncc_inputs.set(PortKey("reference"), PortValue::image(same_pixels));
    const ToolExecutionOutcome correlated =
        harness.run("nvision.tool.measurement.ncc", "node.ncc", std::move(ncc_inputs));
    NVISION_REQUIRE(correlated.is_success());
    NVISION_REQUIRE(
        std::abs(correlated.outputs().find(PortKey("score"))->floating_point_value() - 1.0) < 1e-5);

    PortValueMap ssim_inputs;
    ssim_inputs.set(PortKey("image"), PortValue::image(structured));
    ssim_inputs.set(PortKey("reference"), PortValue::image(same_pixels));
    const ToolExecutionOutcome similar =
        harness.run("nvision.tool.measurement.ssim", "node.ssim", std::move(ssim_inputs));
    NVISION_REQUIRE(similar.is_success());
    NVISION_REQUIRE(
        std::abs(similar.outputs().find(PortKey("score"))->floating_point_value() - 1.0) < 1e-9);

    // Two rasters of different extents describe different things and cannot be compared.
    PortValueMap mismatched;
    mismatched.set(PortKey("image"), PortValue::image(structured));
    mismatched.set(PortKey("reference"), PortValue::image(constant_mono("test.small", 8U, 10U)));
    NVISION_REQUIRE(
        refusal(harness.run(
            "nvision.tool.measurement.ssim", "node.ssim-size", std::move(mismatched))) ==
        ResultCode("SIZE_MISMATCH"));
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

void the_location_and_geometry_group_is_fully_implemented() {
    const ToolRegistry definitions = create_builtin_registry();
    const ToolImplementationRegistry implementations = create_vision_implementation_registry();

    std::size_t geometry_definitions = 0U;
    std::size_t geometry_implemented = 0U;
    for (const auto& definition : definitions.definitions()) {
        if (definition->id().value().rfind("nvision.tool.geometry.", 0U) != 0U) {
            continue;
        }
        ++geometry_definitions;
        if (implementations.contains(definition->id(), definition->version())) {
            ++geometry_implemented;
        }
    }

    NVISION_REQUIRE(geometry_definitions == 11U);
    NVISION_REQUIRE(geometry_implemented == geometry_definitions);
}

void every_measurement_tool_except_grid_deviation_is_implemented() {
    const ToolRegistry definitions = create_builtin_registry();
    const ToolImplementationRegistry implementations = create_vision_implementation_registry();

    std::size_t measurement_definitions = 0U;
    std::size_t measurement_implemented = 0U;
    for (const auto& definition : definitions.definitions()) {
        if (definition->id().value().rfind("nvision.tool.measurement.", 0U) != 0U) {
            continue;
        }
        ++measurement_definitions;
        if (implementations.contains(definition->id(), definition->version())) {
            ++measurement_implemented;
        }
    }

    NVISION_REQUIRE(measurement_definitions == 11U);
    // Grid deviation measures against a point-grid model, a contract that belongs with the
    // point-grid tools, so it is implemented alongside them rather than guessed at here.
    NVISION_REQUIRE(measurement_implemented == 10U);
    NVISION_REQUIRE(!implementations.contains(
        ToolTypeId("nvision.tool.measurement.grid-deviation"), initial_version()));
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
    NVISION_RUN(connected_components_reports_each_region_with_its_own_geometry);
    NVISION_RUN(contour_extraction_and_blob_analysis_agree_on_what_is_there);
    NVISION_RUN(the_minimum_enclosing_circle_covers_its_contour);
    NVISION_RUN(line_fit_recovers_a_line_the_points_lie_on);
    NVISION_RUN(feature_matching_relates_two_rasters_and_refuses_to_match_one_with_itself);
    NVISION_RUN(a_homography_is_estimated_from_correspondences_and_then_judged);
    NVISION_RUN(ecc_refinement_starts_from_a_transform_between_the_two_spaces_it_was_given);
    NVISION_RUN(points_are_transformed_into_the_space_the_transform_names);
    NVISION_RUN(point_assignment_pairs_each_expected_position_at_most_once);
    NVISION_RUN(counting_reports_both_how_many_and_what_share);
    NVISION_RUN(distance_and_radius_are_reported_in_the_space_they_were_measured_in);
    NVISION_RUN(a_histogram_places_every_pixel_in_exactly_one_bin);
    NVISION_RUN(quality_figures_are_zero_for_an_image_with_no_structure);
    NVISION_RUN(comparing_a_raster_with_itself_scores_perfectly);
    NVISION_RUN(the_image_processing_group_is_fully_implemented);
    NVISION_RUN(the_location_and_geometry_group_is_fully_implemented);
    NVISION_RUN(every_measurement_tool_except_grid_deviation_is_implemented);
    return 0;
}
