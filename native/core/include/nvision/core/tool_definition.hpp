#pragma once

#include "nvision/core/data_type.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/parameter_schema.hpp"
#include "nvision/core/semantic_version.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace nvision::core {

enum class PortDirection : std::uint32_t {
    input = 0,
    output = 1,
};

class ToolPortDefinition final {
public:
    ToolPortDefinition(
        PortKey key,
        std::string display_name,
        PortDirection direction,
        PortDataType data_type,
        bool required = false,
        bool allows_multiple = false);

    [[nodiscard]] const PortKey& key() const noexcept;
    [[nodiscard]] const std::string& display_name() const noexcept;
    [[nodiscard]] PortDirection direction() const noexcept;
    [[nodiscard]] PortDataType data_type() const noexcept;
    [[nodiscard]] bool required() const noexcept;
    [[nodiscard]] bool allows_multiple() const noexcept;

private:
    PortKey key_;
    std::string display_name_;
    PortDirection direction_;
    PortDataType data_type_;
    bool required_;
    bool allows_multiple_;
};

class ToolCatalogMetadata final {
public:
    ToolCatalogMetadata(
        std::string display_name,
        std::vector<std::string> category_path,
        std::vector<std::string> tags = {},
        std::string description = {});

    [[nodiscard]] const std::string& display_name() const noexcept;
    [[nodiscard]] const std::vector<std::string>& category_path() const noexcept;
    [[nodiscard]] const std::vector<std::string>& tags() const noexcept;
    [[nodiscard]] const std::string& description() const noexcept;

private:
    std::string display_name_;
    std::vector<std::string> category_path_;
    std::vector<std::string> tags_;
    std::string description_;
};

enum class ToolOriginKind : std::uint32_t {
    built_in = 0,
    plugin = 1,
};

class ToolOrigin final {
public:
    [[nodiscard]] static ToolOrigin built_in();
    [[nodiscard]] static ToolOrigin plugin(PluginId plugin_id);

    [[nodiscard]] ToolOriginKind kind() const noexcept;
    [[nodiscard]] const std::optional<PluginId>& plugin_id() const noexcept;

private:
    ToolOrigin(ToolOriginKind kind, std::optional<PluginId> plugin_id);

    ToolOriginKind kind_;
    std::optional<PluginId> plugin_id_;
};

enum class ToolExecutionLifetime : std::uint32_t {
    stateless = 0,
    per_execution = 1,
    per_session = 2,
    singleton_shared = 3,
};

enum class ToolDeviceRequirement : std::uint32_t {
    cpu = 0,
    gpu_optional = 1,
    gpu_required = 2,
    external_device = 3,
};

class ToolExecutionCharacteristics final {
public:
    ToolExecutionCharacteristics(
        ToolExecutionLifetime lifetime = ToolExecutionLifetime::stateless,
        ToolDeviceRequirement device_requirement = ToolDeviceRequirement::cpu,
        bool thread_safe = false,
        bool reentrant = false,
        bool deterministic = true,
        bool supports_cancellation = true,
        bool supports_timeout = true,
        bool supports_debug_preview = false);

    [[nodiscard]] ToolExecutionLifetime lifetime() const noexcept;
    [[nodiscard]] ToolDeviceRequirement device_requirement() const noexcept;
    [[nodiscard]] bool thread_safe() const noexcept;
    [[nodiscard]] bool reentrant() const noexcept;
    [[nodiscard]] bool deterministic() const noexcept;
    [[nodiscard]] bool supports_cancellation() const noexcept;
    [[nodiscard]] bool supports_timeout() const noexcept;
    [[nodiscard]] bool supports_debug_preview() const noexcept;

private:
    ToolExecutionLifetime lifetime_;
    ToolDeviceRequirement device_requirement_;
    bool thread_safe_;
    bool reentrant_;
    bool deterministic_;
    bool supports_cancellation_;
    bool supports_timeout_;
    bool supports_debug_preview_;
};

class ToolTypeDefinition final {
public:
    ToolTypeDefinition(
        ToolTypeId id,
        SemanticVersion version,
        ToolCatalogMetadata catalog,
        std::vector<ToolPortDefinition> ports,
        ToolOrigin origin = ToolOrigin::built_in(),
        ToolExecutionCharacteristics execution = {},
        std::vector<ParameterDefinition> parameters = {});

    [[nodiscard]] const ToolTypeId& id() const noexcept;
    [[nodiscard]] const SemanticVersion& version() const noexcept;
    [[nodiscard]] const ToolCatalogMetadata& catalog() const noexcept;
    [[nodiscard]] const std::vector<ToolPortDefinition>& ports() const noexcept;
    [[nodiscard]] const ToolOrigin& origin() const noexcept;
    [[nodiscard]] const ToolExecutionCharacteristics& execution() const noexcept;
    [[nodiscard]] const std::vector<ParameterDefinition>& parameters() const noexcept;
    [[nodiscard]] const ToolPortDefinition* find_port(
        const PortKey& key,
        PortDirection direction) const noexcept;
    [[nodiscard]] const ParameterDefinition* find_parameter(const ParameterKey& key) const noexcept;

private:
    ToolTypeId id_;
    SemanticVersion version_;
    ToolCatalogMetadata catalog_;
    std::vector<ToolPortDefinition> ports_;
    ToolOrigin origin_;
    ToolExecutionCharacteristics execution_;
    std::vector<ParameterDefinition> parameters_;
};

}  // namespace nvision::core
