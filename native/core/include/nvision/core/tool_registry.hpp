#pragma once

#include "nvision/core/tool_definition.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nvision::core {

enum class ToolResolutionStatus : std::uint32_t {
    resolved = 0,
    unknown_tool = 1,
    unsupported_version = 2,
};

class ToolResolution final {
public:
    ToolResolution(
        ToolTypeId requested_id,
        SemanticVersion requested_version,
        ToolResolutionStatus status,
        std::shared_ptr<const ToolTypeDefinition> definition = {});

    [[nodiscard]] const ToolTypeId& requested_id() const noexcept;
    [[nodiscard]] const SemanticVersion& requested_version() const noexcept;
    [[nodiscard]] ToolResolutionStatus status() const noexcept;
    [[nodiscard]] bool is_resolved() const noexcept;
    [[nodiscard]] const std::shared_ptr<const ToolTypeDefinition>& definition() const noexcept;

private:
    ToolTypeId requested_id_;
    SemanticVersion requested_version_;
    ToolResolutionStatus status_;
    std::shared_ptr<const ToolTypeDefinition> definition_;
};

class ToolRegistry final {
public:
    [[nodiscard]] std::shared_ptr<const ToolTypeDefinition> register_definition(
        ToolTypeDefinition definition);
    [[nodiscard]] ToolResolution resolve_exact(
        const ToolTypeId& id,
        const SemanticVersion& version) const;
    [[nodiscard]] const std::vector<std::shared_ptr<const ToolTypeDefinition>>& definitions()
        const noexcept;
    [[nodiscard]] std::vector<SemanticVersion> available_versions(const ToolTypeId& id) const;

private:
    [[nodiscard]] static std::string key(
        const ToolTypeId& id,
        const SemanticVersion& version);

    std::map<std::string, std::shared_ptr<const ToolTypeDefinition>> definitions_by_key_;
    std::vector<std::shared_ptr<const ToolTypeDefinition>> definitions_;
};

}  // namespace nvision::core
