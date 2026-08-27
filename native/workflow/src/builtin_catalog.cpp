#include "nvision/workflow/builtin_catalog.hpp"

#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/tool_definition.hpp"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace nvision::workflow {

namespace {

using Ports = std::vector<ToolPortDefinition>;
using Parameters = std::vector<ParameterDefinition>;

const core::SemanticVersion initial_version(1U, 0U, 0U);

// Keys are lowercase and hyphenated; the catalog shows a readable form of the same word.
std::string title_case(const std::string& key) {
    std::string display;
    display.reserve(key.size());
    bool capitalize = true;
    for (const char character : key) {
        if (character == '-' || character == '.' || character == '_') {
            display.push_back(' ');
            capitalize = true;
            continue;
        }
        const auto value = static_cast<unsigned char>(character);
        display.push_back(
            capitalize ? static_cast<char>(std::toupper(value)) : static_cast<char>(value));
        capitalize = false;
    }
    return display;
}

ToolPortDefinition input_port(
    std::string key,
    const core::PortDataType type,
    const bool required = true,
    const bool allows_multiple = false) {
    std::string display = title_case(key);
    return ToolPortDefinition(
        core::PortKey(std::move(key)),
        std::move(display),
        PortDirection::input,
        type,
        required,
        allows_multiple);
}

ToolPortDefinition output_port(std::string key, const core::PortDataType type) {
    std::string display = title_case(key);
    return ToolPortDefinition(
        core::PortKey(std::move(key)), std::move(display), PortDirection::output, type, true);
}

ParameterDefinition boolean_parameter(std::string key, const bool default_value) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::boolean(),
        true,
        ParameterValue::boolean(default_value));
}

ParameterDefinition integer_parameter(
    std::string key,
    const std::int64_t default_value,
    const double minimum,
    const double maximum) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::integer(NumericConstraint(minimum, maximum)),
        true,
        ParameterValue::integer(default_value));
}

ParameterDefinition required_integer_parameter(
    std::string key,
    const double minimum,
    const double maximum) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::integer(NumericConstraint(minimum, maximum)),
        true);
}

ParameterDefinition float_parameter(
    std::string key,
    const double default_value,
    const double minimum,
    const double maximum) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::floating_point(NumericConstraint(minimum, maximum)),
        true,
        ParameterValue::floating_point(default_value));
}

ParameterDefinition required_float_parameter(
    std::string key,
    const double minimum,
    const double maximum) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::floating_point(NumericConstraint(minimum, maximum)),
        true);
}

ParameterDefinition string_parameter(std::string key, std::string default_value) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::string(),
        true,
        ParameterValue::string(std::move(default_value)));
}

ParameterDefinition required_string_parameter(std::string key) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)), std::move(display), ParameterType::string(), true);
}

ParameterDefinition enum_parameter(
    std::string key,
    std::vector<std::string> options,
    const std::size_t default_index) {
    std::vector<EnumOption> declared;
    declared.reserve(options.size());
    for (const auto& option : options) {
        declared.emplace_back(core::EnumOptionKey(option), title_case(option));
    }
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::enumeration(std::move(declared)),
        true,
        ParameterValue::enumeration(core::EnumOptionKey(options[default_index])));
}

ParameterDefinition float_array_parameter(
    std::string key,
    std::vector<double> default_values,
    const std::size_t minimum_items) {
    std::vector<ParameterValue> defaults;
    defaults.reserve(default_values.size());
    for (const double value : default_values) {
        defaults.push_back(ParameterValue::floating_point(value));
    }
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::array(ParameterType::floating_point(), minimum_items),
        true,
        ParameterValue::array(std::move(defaults)));
}

ParameterDefinition string_array_parameter(std::string key, const std::size_t minimum_items) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::array(ParameterType::string(), minimum_items),
        true,
        minimum_items == 0U ? std::optional<ParameterValue>(ParameterValue::array({}))
                            : std::nullopt);
}

ParameterDefinition roi_parameter(std::string key) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::region_of_interest(
            {GeometryKind::rectangle, GeometryKind::polygon, GeometryKind::circle}),
        true);
}

ParameterDefinition model_parameter(std::string key) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)), std::move(display), ParameterType::model_reference(), true);
}

ParameterDefinition resource_parameter(std::string key, const bool required) {
    std::string display = title_case(key);
    return ParameterDefinition(
        core::ParameterKey(std::move(key)),
        std::move(display),
        ParameterType::resource_reference(),
        required);
}

void add(
    ToolRegistry& registry,
    std::string id,
    std::string display_name,
    std::vector<std::string> category_path,
    Ports ports,
    Parameters parameters = {}) {
    static_cast<void>(registry.register_definition(ToolTypeDefinition(
        core::ToolTypeId(std::move(id)),
        initial_version,
        ToolCatalogMetadata(std::move(display_name), std::move(category_path)),
        std::move(ports),
        ToolOrigin::built_in(),
        {},
        std::move(parameters))));
}

void register_acquisition_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Acquisition"};

    add(registry,
        "nvision.tool.acquisition.image-source",
        "Image Source",
        category,
        {output_port("image", core::PortDataType::image_view)},
        {string_parameter("capture-role", "")});

    add(registry,
        "nvision.tool.acquisition.file-camera",
        "File Camera",
        category,
        {output_port("image", core::PortDataType::image_view)},
        {string_parameter("capture-role", ""), integer_parameter("frame-index", 0, 0.0, 1e9)});

    add(registry,
        "nvision.tool.acquisition.capture-sequence",
        "Capture Sequence",
        category,
        {output_port("images", core::PortDataType::image_view)},
        {string_array_parameter("capture-roles", 1U)});

    add(registry,
        "nvision.tool.acquisition.image-buffer",
        "Image Buffer",
        category,
        {input_port("images", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {string_parameter("capture-role", "")});

    add(registry,
        "nvision.tool.acquisition.image-output",
        "Image Output",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("evidence", core::PortDataType::evidence)},
        {string_parameter("storage-key", "")});
}

void register_processing_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Image Processing"};

    add(registry,
        "nvision.tool.processing.color-convert",
        "Color Convert",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {enum_parameter(
            "conversion",
            {"bayer-gr-to-bgr", "bgr-to-gray", "bgr-to-hsv", "gray-to-bgr"},
            1U)});

    add(registry,
        "nvision.tool.processing.channel-extract",
        "Channel Extract",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {integer_parameter("channel", 0, 0.0, 3.0)});

    // Cropping produces an explicit view-to-source transform, never an implicit one.
    add(registry,
        "nvision.tool.processing.crop",
        "Crop",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view),
         output_port("transform", core::PortDataType::transform)},
        {roi_parameter("roi")});

    add(registry,
        "nvision.tool.processing.resize",
        "Resize",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view),
         output_port("transform", core::PortDataType::transform)},
        {required_integer_parameter("width", 1.0, 65536.0),
         required_integer_parameter("height", 1.0, 65536.0),
         enum_parameter("interpolation", {"nearest", "linear", "cubic", "area"}, 1U)});

    add(registry,
        "nvision.tool.processing.normalize",
        "Normalize",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {float_parameter("minimum", 0.0, -1e9, 1e9), float_parameter("maximum", 255.0, -1e9, 1e9)});

    add(registry,
        "nvision.tool.processing.clahe",
        "CLAHE",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {float_parameter("clip-limit", 2.0, 0.0, 1e3),
         integer_parameter("tile-size", 8, 1.0, 1024.0)});

    add(registry,
        "nvision.tool.processing.gaussian-blur",
        "Gaussian Blur",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {integer_parameter("kernel-size", 3, 1.0, 999.0),
         float_parameter("sigma", 0.0, 0.0, 1e3)});

    add(registry,
        "nvision.tool.processing.sharpen",
        "Sharpen",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("image", core::PortDataType::image_view)},
        {float_parameter("strength", 1.0, 0.0, 1e3)});

    add(registry,
        "nvision.tool.processing.threshold",
        "Threshold",
        category,
        {input_port("image", core::PortDataType::image_view), output_port("mask", core::PortDataType::mask)},
        {float_parameter("value", 128.0, 0.0, 65535.0),
         enum_parameter("mode", {"binary", "binary-inverted", "truncate", "to-zero"}, 0U)});

    add(registry,
        "nvision.tool.processing.adaptive-threshold",
        "Adaptive Threshold",
        category,
        {input_port("image", core::PortDataType::image_view), output_port("mask", core::PortDataType::mask)},
        {integer_parameter("block-size", 11, 3.0, 999.0),
         float_parameter("offset", 0.0, -1e3, 1e3),
         enum_parameter("method", {"mean", "gaussian"}, 1U)});

    add(registry,
        "nvision.tool.processing.color-range",
        "Color Range",
        category,
        {input_port("image", core::PortDataType::image_view), output_port("mask", core::PortDataType::mask)},
        {float_array_parameter("lower", {0.0, 0.0, 0.0}, 1U),
         float_array_parameter("upper", {255.0, 255.0, 255.0}, 1U)});

    add(registry,
        "nvision.tool.processing.morphology",
        "Morphology",
        category,
        {input_port("mask", core::PortDataType::mask), output_port("mask", core::PortDataType::mask)},
        {enum_parameter("operation", {"erode", "dilate", "open", "close"}, 2U),
         integer_parameter("kernel-size", 3, 1.0, 999.0),
         integer_parameter("iterations", 1, 1.0, 100.0)});

    add(registry,
        "nvision.tool.processing.distance-transform",
        "Distance Transform",
        category,
        {input_port("mask", core::PortDataType::mask),
         output_port("image", core::PortDataType::image_view)},
        {enum_parameter("distance-type", {"l1", "l2", "chessboard"}, 1U)});

    add(registry,
        "nvision.tool.processing.perspective-warp",
        "Perspective Warp",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("homography", core::PortDataType::homography),
         output_port("image", core::PortDataType::image_view),
         output_port("transform", core::PortDataType::transform)},
        {integer_parameter("width", 0, 0.0, 65536.0),
         integer_parameter("height", 0, 0.0, 65536.0)});

    add(registry,
        "nvision.tool.processing.affine-transform",
        "Affine Transform",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("transform", core::PortDataType::transform),
         output_port("image", core::PortDataType::image_view)});
}

void register_geometry_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Location and Geometry"};

    add(registry,
        "nvision.tool.geometry.connected-components",
        "Connected Components",
        category,
        {input_port("mask", core::PortDataType::mask),
         output_port("components", core::PortDataType::detection_list),
         output_port("count", core::PortDataType::integer)},
        {enum_parameter("connectivity", {"four", "eight"}, 1U),
         float_parameter("minimum-area", 0.0, 0.0, 1e9)});

    add(registry,
        "nvision.tool.geometry.contour-extraction",
        "Contour Extraction",
        category,
        {input_port("mask", core::PortDataType::mask),
         output_port("contours", core::PortDataType::contour_list)},
        {enum_parameter("mode", {"external", "list", "tree"}, 0U),
         float_parameter("minimum-area", 0.0, 0.0, 1e9)});

    add(registry,
        "nvision.tool.geometry.blob-analysis",
        "Blob Analysis",
        category,
        {input_port("mask", core::PortDataType::mask),
         output_port("blobs", core::PortDataType::detection_list),
         output_port("measurements", core::PortDataType::measurement_list)},
        {float_parameter("minimum-area", 0.0, 0.0, 1e9),
         float_parameter("maximum-area", 0.0, 0.0, 1e9)});

    add(registry,
        "nvision.tool.geometry.minimum-enclosing-circle",
        "Minimum Enclosing Circle",
        category,
        {input_port("contours", core::PortDataType::contour_list),
         output_port("centers", core::PortDataType::point_list),
         output_port("radii", core::PortDataType::measurement_list)});

    add(registry,
        "nvision.tool.geometry.line-fit",
        "Line Fit",
        category,
        {input_port("points", core::PortDataType::point_list),
         output_port("line", core::PortDataType::line_2d),
         output_port("residual", core::PortDataType::floating_point)},
        {enum_parameter("method", {"least-squares", "huber"}, 0U)});

    add(registry,
        "nvision.tool.geometry.feature-detect-and-match",
        "Feature Detect And Match",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("reference", core::PortDataType::image_view),
         output_port("matches", core::PortDataType::match_set),
         output_port("features", core::PortDataType::feature_set)},
        {enum_parameter("backend", {"akaze", "orb", "sift"}, 0U),
         integer_parameter("maximum-features", 2000, 1.0, 1e6),
         float_parameter("ratio-threshold", 0.75, 0.0, 1.0)});

    add(registry,
        "nvision.tool.geometry.homography-estimate",
        "Homography Estimate",
        category,
        {input_port("matches", core::PortDataType::match_set),
         output_port("homography", core::PortDataType::homography),
         output_port("inlier-ratio", core::PortDataType::floating_point)},
        {enum_parameter("method", {"ransac", "lmeds"}, 0U),
         float_parameter("reprojection-threshold", 3.0, 0.0, 1e3)});

    add(registry,
        "nvision.tool.geometry.homography-validate",
        "Homography Validate",
        category,
        {input_port("homography", core::PortDataType::homography),
         output_port("valid", core::PortDataType::boolean),
         output_port("measurements", core::PortDataType::measurement_list)},
        {float_parameter("minimum-determinant", 0.1, 0.0, 1e6),
         float_parameter("maximum-scale-deviation", 0.5, 0.0, 1e3)});

    add(registry,
        "nvision.tool.geometry.ecc-refine",
        "ECC Refine",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("reference", core::PortDataType::image_view),
         input_port("transform", core::PortDataType::transform),
         output_port("transform", core::PortDataType::transform),
         output_port("correlation", core::PortDataType::floating_point)},
        {integer_parameter("iterations", 50, 1.0, 1e5),
         float_parameter("epsilon", 0.0001, 0.0, 1.0)});

    add(registry,
        "nvision.tool.geometry.point-transform",
        "Point Transform",
        category,
        {input_port("points", core::PortDataType::point_list),
         input_port("transform", core::PortDataType::transform),
         output_port("points", core::PortDataType::point_list)});

    add(registry,
        "nvision.tool.geometry.point-assignment",
        "Point Assignment",
        category,
        {input_port("points", core::PortDataType::point_list),
         input_port("reference", core::PortDataType::point_list),
         output_port("assignments", core::PortDataType::measurement_list),
         output_port("unmatched", core::PortDataType::point_list)},
        {float_parameter("maximum-distance", 10.0, 0.0, 1e6)});
}

void register_measurement_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Measurement"};

    add(registry,
        "nvision.tool.measurement.pixel-count",
        "Pixel Count",
        category,
        {input_port("mask", core::PortDataType::mask),
         output_port("count", core::PortDataType::integer),
         output_port("ratio", core::PortDataType::floating_point)});

    add(registry,
        "nvision.tool.measurement.object-count",
        "Object Count",
        category,
        {input_port("objects", core::PortDataType::detection_list),
         output_port("count", core::PortDataType::integer)});

    add(registry,
        "nvision.tool.measurement.distance",
        "Distance",
        category,
        {input_port("from", core::PortDataType::point_2d),
         input_port("to", core::PortDataType::point_2d),
         output_port("measurement", core::PortDataType::measurement)});

    add(registry,
        "nvision.tool.measurement.radius-diameter",
        "Radius Diameter",
        category,
        {input_port("circle", core::PortDataType::circle),
         output_port("measurements", core::PortDataType::measurement_list)});

    add(registry,
        "nvision.tool.measurement.histogram",
        "Histogram",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("histogram", core::PortDataType::measurement_list)},
        {integer_parameter("bins", 256, 1.0, 65536.0)});

    add(registry,
        "nvision.tool.measurement.sharpness",
        "Sharpness",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("measurement", core::PortDataType::measurement)},
        {enum_parameter("method", {"laplacian-variance", "tenengrad"}, 0U)});

    add(registry,
        "nvision.tool.measurement.contrast",
        "Contrast",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("measurement", core::PortDataType::measurement)});

    add(registry,
        "nvision.tool.measurement.edge-density",
        "Edge Density",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("measurement", core::PortDataType::measurement)},
        {float_parameter("low-threshold", 50.0, 0.0, 65535.0),
         float_parameter("high-threshold", 150.0, 0.0, 65535.0)});

    add(registry,
        "nvision.tool.measurement.ncc",
        "NCC",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("reference", core::PortDataType::image_view),
         output_port("score", core::PortDataType::floating_point)});

    add(registry,
        "nvision.tool.measurement.ssim",
        "SSIM",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("reference", core::PortDataType::image_view),
         output_port("score", core::PortDataType::floating_point)});

    add(registry,
        "nvision.tool.measurement.grid-deviation",
        "Grid Deviation",
        category,
        {input_port("points", core::PortDataType::point_list),
         input_port("reference", core::PortDataType::point_grid_model),
         output_port("measurements", core::PortDataType::measurement_list),
         output_port("maximum", core::PortDataType::floating_point)});
}

void register_recognition_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Recognition and Text"};

    add(registry,
        "nvision.tool.recognition.text-detection",
        "Text Detection",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("regions", core::PortDataType::detection_list)},
        {model_parameter("model"), float_parameter("minimum-score", 0.5, 0.0, 1.0)});

    // Orientation search is configuration, so multi-orientation reading stays a Recipe
    // decision rather than a separate built-in tool.
    add(registry,
        "nvision.tool.recognition.text-recognition",
        "Text Recognition",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("regions", core::PortDataType::detection_list, false),
         output_port("text", core::PortDataType::text_result),
         output_port("crops", core::PortDataType::image_view)},
        {model_parameter("model"),
         resource_parameter("dictionary", false),
         float_array_parameter("orientations", {0.0}, 1U),
         float_parameter("minimum-confidence", 0.5, 0.0, 1.0)});

    add(registry,
        "nvision.tool.recognition.ocr",
        "OCR",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("text", core::PortDataType::text_result)},
        {model_parameter("model"), float_parameter("minimum-confidence", 0.5, 0.0, 1.0)});

    add(registry,
        "nvision.tool.recognition.barcode-reader",
        "Barcode Reader",
        category,
        {input_port("image", core::PortDataType::image_view),
         output_port("barcodes", core::PortDataType::barcode_result)},
        {string_array_parameter("symbologies", 0U)});

    add(registry,
        "nvision.tool.recognition.text-normalize",
        "Text Normalize",
        category,
        {input_port("text", core::PortDataType::text_result),
         output_port("text", core::PortDataType::text_result),
         output_port("value", core::PortDataType::string)},
        {boolean_parameter("uppercase", true),
         boolean_parameter("trim-whitespace", true),
         string_parameter("strip-characters", "")});

    add(registry,
        "nvision.tool.recognition.text-split",
        "Text Split",
        category,
        {input_port("text", core::PortDataType::text_result),
         output_port("parts", core::PortDataType::text_result_list)},
        {string_parameter("separator", " "),
         integer_parameter("maximum-parts", 0, 0.0, 1024.0)});

    add(registry,
        "nvision.tool.recognition.string-similarity",
        "String Similarity",
        category,
        {input_port("left", core::PortDataType::text_result),
         input_port("right", core::PortDataType::text_result),
         output_port("score", core::PortDataType::floating_point)},
        {enum_parameter("metric", {"levenshtein", "jaro-winkler"}, 0U)});

    add(registry,
        "nvision.tool.recognition.regex-validate",
        "Regex Validate",
        category,
        {input_port("text", core::PortDataType::text_result),
         output_port("valid", core::PortDataType::boolean)},
        {required_string_parameter("pattern")});

    add(registry,
        "nvision.tool.recognition.text-consensus",
        "Text Consensus",
        category,
        {input_port("candidates", core::PortDataType::text_result_list, true, true),
         output_port("text", core::PortDataType::text_result),
         output_port("value", core::PortDataType::string),
         output_port("confidence", core::PortDataType::floating_point)},
        {float_parameter("minimum-agreement", 0.5, 0.0, 1.0)});
}

void register_structure_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Clustering and Structure"};

    add(registry,
        "nvision.tool.structure.k-means",
        "K-Means",
        category,
        {input_port("points", core::PortDataType::point_list),
         output_port("labels", core::PortDataType::measurement_list),
         output_port("centers", core::PortDataType::point_list)},
        {required_integer_parameter("cluster-count", 1.0, 1e5),
         integer_parameter("iterations", 10, 1.0, 1e5)});

    add(registry,
        "nvision.tool.structure.dbscan",
        "DBSCAN",
        category,
        {input_port("points", core::PortDataType::point_list),
         output_port("labels", core::PortDataType::measurement_list),
         output_port("centers", core::PortDataType::point_list)},
        {float_parameter("epsilon", 10.0, 0.0, 1e6),
         integer_parameter("minimum-samples", 4, 1.0, 1e5)});

    add(registry,
        "nvision.tool.structure.mean-shift",
        "Mean Shift",
        category,
        {input_port("points", core::PortDataType::point_list),
         output_port("centers", core::PortDataType::point_list)},
        {float_parameter("bandwidth", 10.0, 0.0, 1e6)});

    add(registry,
        "nvision.tool.structure.sort-points-along-line",
        "Sort Points Along Line",
        category,
        {input_port("points", core::PortDataType::point_list),
         input_port("line", core::PortDataType::line_2d),
         output_port("points", core::PortDataType::point_list)},
        {boolean_parameter("descending", false)});

    add(registry,
        "nvision.tool.structure.point-grid-learn",
        "Point Grid Learn",
        category,
        {input_port("points", core::PortDataType::point_list),
         output_port("model", core::PortDataType::point_grid_model)},
        {float_parameter("row-tolerance", 5.0, 0.0, 1e6),
         float_parameter("column-tolerance", 5.0, 0.0, 1e6)});

    // The learned grid arrives as a versioned resource, so training and inspection stay
    // separate runs joined by a Recipe binding rather than by one live connection.
    add(registry,
        "nvision.tool.structure.point-grid-inspect",
        "Point Grid Inspect",
        category,
        {input_port("points", core::PortDataType::point_list),
         output_port("result", core::PortDataType::point_grid_result),
         output_port("missing", core::PortDataType::point_list),
         output_port("extra", core::PortDataType::point_list)},
        {resource_parameter("model", true), float_parameter("position-tolerance", 5.0, 0.0, 1e6)});
}

void register_logic_tools(ToolRegistry& registry) {
    const std::vector<std::string> category{"Logic and Results"};

    add(registry,
        "nvision.tool.logic.lookup-table",
        "Lookup Table",
        category,
        {input_port("key", core::PortDataType::string),
         output_port("value", core::PortDataType::string),
         output_port("found", core::PortDataType::boolean)},
        {string_array_parameter("keys", 1U),
         string_array_parameter("values", 1U),
         string_parameter("fallback-value", "")});

    add(registry,
        "nvision.tool.logic.expression",
        "Expression",
        category,
        {input_port("inputs", core::PortDataType::floating_point, true, true),
         output_port("value", core::PortDataType::floating_point)},
        {required_string_parameter("expression")});

    add(registry,
        "nvision.tool.logic.weighted-score",
        "Weighted Score",
        category,
        {input_port("values", core::PortDataType::measurement, true, true),
         output_port("score", core::PortDataType::floating_point)},
        {float_array_parameter("weights", {1.0}, 1U)});

    add(registry,
        "nvision.tool.logic.compare",
        "Compare",
        category,
        {input_port("value", core::PortDataType::floating_point),
         output_port("passed", core::PortDataType::boolean),
         output_port("decision", core::PortDataType::decision)},
        {enum_parameter(
             "operation",
             {"less-than",
              "less-or-equal",
              "greater-than",
              "greater-or-equal",
              "equal",
              "not-equal"},
             3U),
         required_float_parameter("threshold", -1e12, 1e12)});

    add(registry,
        "nvision.tool.logic.boolean-logic",
        "Boolean Logic",
        category,
        {input_port("operands", core::PortDataType::boolean, true, true),
         output_port("result", core::PortDataType::boolean)},
        {enum_parameter("operation", {"all", "any", "none", "exactly-one"}, 0U)});

    add(registry,
        "nvision.tool.logic.branch",
        "Branch",
        category,
        {input_port("condition", core::PortDataType::boolean),
         input_port("value", core::PortDataType::any_metadata),
         output_port("when-true", core::PortDataType::any_metadata),
         output_port("when-false", core::PortDataType::any_metadata)});

    add(registry,
        "nvision.tool.logic.decision",
        "Decision",
        category,
        {input_port("passed", core::PortDataType::boolean),
         output_port("decision", core::PortDataType::decision)},
        {string_parameter("pass-code", "OK"),
         string_parameter("fail-code", "NG"),
         float_parameter("confidence", 1.0, 0.0, 1.0)});

    add(registry,
        "nvision.tool.logic.result-builder",
        "Result Builder",
        category,
        {input_port("decision", core::PortDataType::decision),
         input_port("measurements", core::PortDataType::measurement, false, true),
         input_port("evidence", core::PortDataType::evidence, false, true),
         output_port("result", core::PortDataType::any_metadata)});

    add(registry,
        "nvision.tool.logic.evidence-output",
        "Evidence Output",
        category,
        {input_port("image", core::PortDataType::image_view),
         input_port("overlay", core::PortDataType::overlay, false),
         output_port("evidence", core::PortDataType::evidence)},
        {string_parameter("storage-key", "")});
}

}  // namespace

void register_builtin_tools(ToolRegistry& registry) {
    register_acquisition_tools(registry);
    register_processing_tools(registry);
    register_geometry_tools(registry);
    register_measurement_tools(registry);
    register_recognition_tools(registry);
    register_structure_tools(registry);
    register_logic_tools(registry);
}

ToolRegistry create_builtin_registry() {
    ToolRegistry registry;
    register_builtin_tools(registry);
    return registry;
}

}  // namespace nvision::workflow
