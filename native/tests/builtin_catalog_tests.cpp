#include "nvision/workflow/builtin_catalog.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                            \
        if (!(condition)) {         \
            std::abort();           \
        }                           \
    } while (false)

namespace {

using nvision::core::ParameterKey;
using nvision::workflow::ParameterKind;
using nvision::workflow::PortDirection;
using nvision::core::PortKey;
using nvision::core::SemanticVersion;
using nvision::workflow::ToolOriginKind;
using nvision::workflow::ToolRegistry;
using nvision::workflow::ToolResolutionStatus;
using nvision::core::ToolTypeId;

std::size_t count_in_category(const ToolRegistry& registry, const std::string& category) {
    std::size_t total = 0U;
    for (const auto& definition : registry.definitions()) {
        if (definition->catalog().category_path().front() == category) {
            ++total;
        }
    }
    return total;
}

bool contains(const std::string& text, const std::string& token) {
    std::string lowered;
    lowered.reserve(text.size());
    for (const char character : text) {
        const auto value = static_cast<unsigned char>(character);
        lowered.push_back(
            value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a')
                                         : static_cast<char>(value));
    }
    return lowered.find(token) != std::string::npos;
}

void catalog_enumerates_the_initial_generic_tool_tree() {
    const ToolRegistry registry = nvision::workflow::create_builtin_registry();

    NVISION_REQUIRE(registry.definitions().size() == 66U);
    NVISION_REQUIRE(count_in_category(registry, "Acquisition") == 5U);
    NVISION_REQUIRE(count_in_category(registry, "Image Processing") == 15U);
    NVISION_REQUIRE(count_in_category(registry, "Location and Geometry") == 11U);
    NVISION_REQUIRE(count_in_category(registry, "Measurement") == 11U);
    NVISION_REQUIRE(count_in_category(registry, "Recognition and Text") == 9U);
    NVISION_REQUIRE(count_in_category(registry, "Clustering and Structure") == 6U);
    NVISION_REQUIRE(count_in_category(registry, "Logic and Results") == 9U);

    const std::vector<std::string> sample{
        "nvision.tool.acquisition.capture-sequence",
        "nvision.tool.processing.morphology",
        "nvision.tool.geometry.homography-estimate",
        "nvision.tool.measurement.ssim",
        "nvision.tool.recognition.text-consensus",
        "nvision.tool.structure.point-grid-inspect",
        "nvision.tool.logic.result-builder"};
    for (const auto& id : sample) {
        const auto resolution =
            registry.resolve_exact(ToolTypeId(id), SemanticVersion(1U, 0U, 0U));
        NVISION_REQUIRE(resolution.is_resolved());
        NVISION_REQUIRE(resolution.definition()->id().value() == id);
    }

    const auto unknown = registry.resolve_exact(
        ToolTypeId("nvision.tool.processing.deskew"), SemanticVersion(1U, 0U, 0U));
    NVISION_REQUIRE(unknown.status() == ToolResolutionStatus::unknown_tool);
}

void catalog_definitions_are_built_in_and_typed() {
    const ToolRegistry registry = nvision::workflow::create_builtin_registry();

    for (const auto& definition : registry.definitions()) {
        NVISION_REQUIRE(definition->origin().kind() == ToolOriginKind::built_in);
        NVISION_REQUIRE(!definition->origin().plugin_id().has_value());
        NVISION_REQUIRE(definition->version().to_string() == "1.0.0");
        NVISION_REQUIRE(!definition->catalog().category_path().empty());
        NVISION_REQUIRE(!definition->catalog().display_name().empty());

        const bool has_output = std::any_of(
            definition->ports().begin(),
            definition->ports().end(),
            [](const auto& port) { return port.direction() == PortDirection::output; });
        NVISION_REQUIRE(has_output);
    }
}

// Guards the boundary in the execution plan: no generic contract may carry the vocabulary
// of a reference application. Applications are graphs and Recipes, not built-in tools.
void catalog_contains_no_application_vocabulary() {
    const ToolRegistry registry = nvision::workflow::create_builtin_registry();
    const std::vector<std::string> forbidden{
        "wire", "marker", "pininsert", "pin-insert", "pin insert"};

    for (const auto& definition : registry.definitions()) {
        std::vector<std::string> text{definition->id().value(),
                                      definition->catalog().display_name(),
                                      definition->catalog().description()};
        for (const auto& segment : definition->catalog().category_path()) {
            text.push_back(segment);
        }
        for (const auto& tag : definition->catalog().tags()) {
            text.push_back(tag);
        }
        for (const auto& port : definition->ports()) {
            text.push_back(port.key().value());
            text.push_back(port.display_name());
        }
        for (const auto& parameter : definition->parameters()) {
            text.push_back(parameter.key().value());
            text.push_back(parameter.display_name());
        }

        for (const auto& candidate : text) {
            for (const auto& token : forbidden) {
                NVISION_REQUIRE(!contains(candidate, token));
            }
        }
    }
}

// Capabilities that a reference application might expect as dedicated tools must stay
// configuration of a generic tool instead.
// Metadata is a promise the engine and Studio act on, so it has to match the code. No
// built-in implementation observes the cancellation token while it runs, and none bounds
// its own work to the budget, so no built-in definition may claim either. This test is
// meant to fail the day a tool starts claiming one, so that the claim and the cooperative
// checkpoints that justify it are added together.
void catalog_execution_characteristics_match_the_implementations() {
    const ToolRegistry registry = nvision::workflow::create_builtin_registry();
    NVISION_REQUIRE(!registry.definitions().empty());
    for (const auto& definition : registry.definitions()) {
        NVISION_REQUIRE(!definition->execution().supports_cancellation());
        NVISION_REQUIRE(!definition->execution().supports_timeout());
    }
}

void application_variation_stays_configuration() {
    const ToolRegistry registry = nvision::workflow::create_builtin_registry();

    const auto recognition = registry.resolve_exact(
        ToolTypeId("nvision.tool.recognition.text-recognition"), SemanticVersion(1U, 0U, 0U));
    const auto* orientations =
        recognition.definition()->find_parameter(ParameterKey("orientations"));
    NVISION_REQUIRE(orientations != nullptr);
    NVISION_REQUIRE(orientations->type().kind() == ParameterKind::array);
    NVISION_REQUIRE(
        recognition.definition()->find_parameter(ParameterKey("model"))->type().kind() ==
        ParameterKind::model_reference);

    // A learned grid arrives as a versioned resource, so training and inspection are two
    // runs joined by a Recipe binding rather than one connected graph.
    const auto inspect = registry.resolve_exact(
        ToolTypeId("nvision.tool.structure.point-grid-inspect"), SemanticVersion(1U, 0U, 0U));
    const auto* model = inspect.definition()->find_parameter(ParameterKey("model"));
    NVISION_REQUIRE(model != nullptr);
    NVISION_REQUIRE(model->type().kind() == ParameterKind::resource_reference);
    NVISION_REQUIRE(model->required());
    NVISION_REQUIRE(!model->default_value().has_value());
    NVISION_REQUIRE(
        inspect.definition()->find_port(PortKey("model"), PortDirection::input) == nullptr);

    // A backend choice is a parameter, not a separate tool per algorithm.
    const auto matcher = registry.resolve_exact(
        ToolTypeId("nvision.tool.geometry.feature-detect-and-match"),
        SemanticVersion(1U, 0U, 0U));
    const auto* backend = matcher.definition()->find_parameter(ParameterKey("backend"));
    NVISION_REQUIRE(backend != nullptr);
    NVISION_REQUIRE(backend->type().enum_options().size() == 3U);
}

}  // namespace

int main() {
    catalog_enumerates_the_initial_generic_tool_tree();
    catalog_definitions_are_built_in_and_typed();
    catalog_contains_no_application_vocabulary();
    catalog_execution_characteristics_match_the_implementations();
    application_variation_stays_configuration();
    return EXIT_SUCCESS;
}
