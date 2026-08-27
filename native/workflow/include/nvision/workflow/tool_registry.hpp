#pragma once

#include "nvision/workflow/tool_definition.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nvision::workflow {

enum class ToolResolutionStatus : std::uint32_t {
    resolved = 0,
    unknown_tool = 1,
    unsupported_version = 2,
};

class ToolResolution final {
public:
    ToolResolution(
        core::ToolTypeId requested_id,
        core::SemanticVersion requested_version,
        ToolResolutionStatus status,
        std::shared_ptr<const ToolTypeDefinition> definition = {});

    [[nodiscard]] const core::ToolTypeId& requested_id() const noexcept;
    [[nodiscard]] const core::SemanticVersion& requested_version() const noexcept;
    [[nodiscard]] ToolResolutionStatus status() const noexcept;
    [[nodiscard]] bool is_resolved() const noexcept;
    [[nodiscard]] const std::shared_ptr<const ToolTypeDefinition>& definition() const noexcept;

private:
    core::ToolTypeId requested_id_;
    core::SemanticVersion requested_version_;
    ToolResolutionStatus status_;
    std::shared_ptr<const ToolTypeDefinition> definition_;
};

class ToolRegistry final {
public:
    [[nodiscard]] std::shared_ptr<const ToolTypeDefinition> register_definition(
        ToolTypeDefinition definition);
    [[nodiscard]] ToolResolution resolve_exact(
        const core::ToolTypeId& id,
        const core::SemanticVersion& version) const;
    [[nodiscard]] const std::vector<std::shared_ptr<const ToolTypeDefinition>>& definitions()
        const noexcept;
    [[nodiscard]] std::vector<core::SemanticVersion> available_versions(const core::ToolTypeId& id) const;

private:
    [[nodiscard]] static std::string key(
        const core::ToolTypeId& id,
        const core::SemanticVersion& version);

    std::map<std::string, std::shared_ptr<const ToolTypeDefinition>> definitions_by_key_;
    std::vector<std::shared_ptr<const ToolTypeDefinition>> definitions_;
};

}  // namespace nvision::workflow
