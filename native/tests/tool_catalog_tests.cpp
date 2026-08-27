#include "nvision/workflow/tool_registry.hpp"

#include <cstdlib>
#include <memory>
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

using nvision::core::PluginId;
using nvision::core::PortDataType;
using nvision::workflow::PortDirection;
using nvision::core::PortKey;
using nvision::core::SemanticVersion;
using nvision::workflow::ToolCatalogMetadata;
using nvision::workflow::ToolDeviceRequirement;
using nvision::workflow::ToolExecutionCharacteristics;
using nvision::workflow::ToolExecutionLifetime;
using nvision::workflow::ToolOrigin;
using nvision::workflow::ToolOriginKind;
using nvision::workflow::ToolPortDefinition;
using nvision::workflow::ToolRegistry;
using nvision::workflow::ToolResolution;
using nvision::workflow::ToolResolutionStatus;
using nvision::workflow::ToolTypeDefinition;
using nvision::core::ToolTypeId;

template <typename Action>
bool rejects_invalid_argument(Action action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

ToolPortDefinition input(
    std::string key,
    const PortDataType type,
    const bool required = true) {
    return ToolPortDefinition(
        PortKey(std::move(key)),
        "Input",
        PortDirection::input,
        type,
        required);
}

ToolPortDefinition output(std::string key, const PortDataType type) {
    return ToolPortDefinition(
        PortKey(std::move(key)),
        "Output",
        PortDirection::output,
        type,
        true);
}

ToolTypeDefinition definition(
    std::string id,
    SemanticVersion version,
    std::vector<std::string> category,
    ToolOrigin origin = ToolOrigin::built_in()) {
    return ToolTypeDefinition(
        ToolTypeId(std::move(id)),
        std::move(version),
        ToolCatalogMetadata(
            "Generic Tool",
            std::move(category),
            {"generic", "inspection"},
            "Application-neutral test definition."),
        {input("image", PortDataType::image_view),
         output("result", PortDataType::any_metadata)},
        std::move(origin),
        ToolExecutionCharacteristics(
            ToolExecutionLifetime::per_execution,
            ToolDeviceRequirement::cpu,
            true,
            true,
            true,
            true,
            true,
            true));
}

void tool_definitions_expose_immutable_catalog_contracts() {
    const ToolTypeDefinition tool = definition(
        "nvision.tool.recognition.text-detection",
        SemanticVersion(1U, 2U, 0U),
        {"Recognition", "Text"},
        ToolOrigin::plugin(PluginId("nvision.plugin.ocr")));

    NVISION_REQUIRE(tool.id().value() == "nvision.tool.recognition.text-detection");
    NVISION_REQUIRE(tool.version().to_string() == "1.2.0");
    NVISION_REQUIRE(tool.catalog().display_name() == "Generic Tool");
    NVISION_REQUIRE(tool.catalog().category_path().size() == 2U);
    NVISION_REQUIRE(tool.catalog().category_path()[0] == "Recognition");
    NVISION_REQUIRE(tool.catalog().tags().size() == 2U);
    NVISION_REQUIRE(tool.origin().kind() == ToolOriginKind::plugin);
    NVISION_REQUIRE(tool.origin().plugin_id()->value() == "nvision.plugin.ocr");
    NVISION_REQUIRE(tool.execution().lifetime() == ToolExecutionLifetime::per_execution);
    NVISION_REQUIRE(tool.execution().device_requirement() == ToolDeviceRequirement::cpu);
    NVISION_REQUIRE(tool.execution().thread_safe());
    NVISION_REQUIRE(tool.execution().reentrant());
    NVISION_REQUIRE(tool.execution().deterministic());
    NVISION_REQUIRE(tool.execution().supports_cancellation());
    NVISION_REQUIRE(tool.execution().supports_timeout());
    NVISION_REQUIRE(tool.execution().supports_debug_preview());

    const auto* image = tool.find_port(PortKey("image"), PortDirection::input);
    NVISION_REQUIRE(image != nullptr);
    NVISION_REQUIRE(image->data_type() == PortDataType::image_view);
    NVISION_REQUIRE(image->required());
    NVISION_REQUIRE(!image->allows_multiple());
    NVISION_REQUIRE(tool.find_port(PortKey("image"), PortDirection::output) == nullptr);
}

void registry_resolves_exact_versions_and_preserves_unresolved_identity() {
    ToolRegistry registry;
    const auto version_one = registry.register_definition(definition(
        "nvision.tool.processing.threshold",
        SemanticVersion(1U, 0U, 0U),
        {"Image Processing", "Segmentation"}));
    const auto version_two = registry.register_definition(definition(
        "nvision.tool.processing.threshold",
        SemanticVersion(2U, 0U, 0U),
        {"Image Processing", "Threshold"}));

    const ToolResolution resolved = registry.resolve_exact(
        ToolTypeId("nvision.tool.processing.threshold"),
        SemanticVersion(1U, 0U, 0U));
    NVISION_REQUIRE(resolved.is_resolved());
    NVISION_REQUIRE(resolved.status() == ToolResolutionStatus::resolved);
    NVISION_REQUIRE(resolved.definition() == version_one);
    NVISION_REQUIRE(resolved.requested_id() == version_one->id());
    NVISION_REQUIRE(resolved.requested_version() == version_one->version());

    const ToolResolution unsupported = registry.resolve_exact(
        ToolTypeId("nvision.tool.processing.threshold"),
        SemanticVersion(3U, 0U, 0U));
    NVISION_REQUIRE(!unsupported.is_resolved());
    NVISION_REQUIRE(unsupported.status() == ToolResolutionStatus::unsupported_version);
    NVISION_REQUIRE(!unsupported.definition());
    NVISION_REQUIRE(unsupported.requested_id().value() ==
                    "nvision.tool.processing.threshold");
    NVISION_REQUIRE(unsupported.requested_version().to_string() == "3.0.0");

    const ToolResolution unknown = registry.resolve_exact(
        ToolTypeId("nvision.tool.unknown"),
        SemanticVersion(1U, 0U, 0U));
    NVISION_REQUIRE(unknown.status() == ToolResolutionStatus::unknown_tool);
    NVISION_REQUIRE(unknown.requested_id().value() == "nvision.tool.unknown");

    const auto versions = registry.available_versions(version_one->id());
    NVISION_REQUIRE(versions.size() == 2U);
    NVISION_REQUIRE(versions[0].to_string() == "1.0.0");
    NVISION_REQUIRE(versions[1].to_string() == "2.0.0");
    NVISION_REQUIRE(registry.definitions().size() == 2U);
    NVISION_REQUIRE(registry.definitions()[1] == version_two);
}

void catalog_enumerates_generic_reference_capabilities() {
    ToolRegistry registry;
    const auto capture = registry.register_definition(definition(
        "nvision.tool.acquisition.capture-sequence",
        SemanticVersion(1U, 0U, 0U),
        {"Acquisition"}));
    const auto text = registry.register_definition(definition(
        "nvision.tool.recognition.text-detection",
        SemanticVersion(1U, 0U, 0U),
        {"Recognition", "Text"}));
    const auto grid = registry.register_definition(definition(
        "nvision.tool.structure.point-grid-inspect",
        SemanticVersion(1U, 0U, 0U),
        {"Clustering and Structure", "Point Grid"}));

    NVISION_REQUIRE(registry.definitions().size() == 3U);
    NVISION_REQUIRE(capture->catalog().category_path()[0] == "Acquisition");
    NVISION_REQUIRE(text->catalog().category_path()[0] == "Recognition");
    NVISION_REQUIRE(grid->catalog().category_path()[0] == "Clustering and Structure");
}

void category_metadata_never_becomes_registry_identity() {
    ToolRegistry registry;
    const auto registered = registry.register_definition(definition(
        "nvision.tool.measurement.distance",
        SemanticVersion(1U, 0U, 0U),
        {"Measurement"}));

    NVISION_REQUIRE(rejects_invalid_argument([&registry] {
        const auto duplicate = registry.register_definition(definition(
            "nvision.tool.measurement.distance",
            SemanticVersion(1U, 0U, 0U),
            {"Geometry", "Measurements"}));
        static_cast<void>(duplicate);
    }));

    const auto resolved = registry.resolve_exact(
        ToolTypeId("nvision.tool.measurement.distance"),
        SemanticVersion(1U, 0U, 0U));
    NVISION_REQUIRE(resolved.definition() == registered);
    NVISION_REQUIRE(resolved.definition()->catalog().category_path()[0] == "Measurement");
}

void invalid_definitions_and_resolution_states_are_rejected() {
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolCatalogMetadata invalid("  ", {"Processing"});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolCatalogMetadata invalid("Threshold", {});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolCatalogMetadata invalid("Threshold", {"Processing/Threshold"});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolCatalogMetadata invalid("Threshold", {"Processing"}, {"threshold", "threshold"});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolPortDefinition invalid(
            PortKey("image"),
            "Image",
            static_cast<PortDirection>(999U),
            PortDataType::image);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolPortDefinition invalid(
            PortKey("image"),
            "Image",
            PortDirection::input,
            static_cast<PortDataType>(999U));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolExecutionCharacteristics invalid(static_cast<ToolExecutionLifetime>(999U));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolExecutionCharacteristics invalid(
            ToolExecutionLifetime::stateless,
            static_cast<ToolDeviceRequirement>(999U));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolTypeDefinition invalid(
            ToolTypeId("nvision.tool.invalid-ports"),
            SemanticVersion(1U, 0U, 0U),
            ToolCatalogMetadata("Invalid", {"Testing"}),
            {input("image", PortDataType::image), input("image", PortDataType::mask)});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        const auto mismatched = std::make_shared<const ToolTypeDefinition>(definition(
            "nvision.tool.actual",
            SemanticVersion(1U, 0U, 0U),
            std::vector<std::string>{"Testing"}));
        ToolResolution invalid(
            ToolTypeId("nvision.tool.requested"),
            SemanticVersion(1U, 0U, 0U),
            ToolResolutionStatus::resolved,
            mismatched);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        ToolResolution invalid(
            ToolTypeId("nvision.tool.invalid-resolution"),
            SemanticVersion(1U, 0U, 0U),
            ToolResolutionStatus::resolved);
    }));
}

}  // namespace

int main() {
    tool_definitions_expose_immutable_catalog_contracts();
    registry_resolves_exact_versions_and_preserves_unresolved_identity();
    catalog_enumerates_generic_reference_capabilities();
    category_metadata_never_becomes_registry_identity();
    invalid_definitions_and_resolution_states_are_rejected();
    return EXIT_SUCCESS;
}
