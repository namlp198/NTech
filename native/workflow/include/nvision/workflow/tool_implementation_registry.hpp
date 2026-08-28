#pragma once

#include "nvision/core/identifier.hpp"
#include "nvision/core/semantic_version.hpp"
#include "nvision/workflow/tool_definition.hpp"
#include "nvision/workflow/tool_execution.hpp"
#include "nvision/workflow/tool_registry.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nvision::workflow {

enum class ToolImplementationStatus : std::uint32_t {
    bound = 0,
    unknown_tool = 1,
    unsupported_version = 2,
    not_implemented = 3,
};

// The result of binding one requested tool identity to a native implementation. A
// definition that exists in the catalog but has no implementation yet is reported as
// not_implemented, never as a missing tool, so catalog coverage stays visible.
class ToolImplementationBinding final {
public:
    ToolImplementationBinding(
        core::ToolTypeId requested_id,
        core::SemanticVersion requested_version,
        ToolImplementationStatus status,
        std::shared_ptr<const ToolTypeDefinition> definition,
        std::shared_ptr<const ToolImplementation> implementation);

    [[nodiscard]] const core::ToolTypeId& requested_id() const noexcept;
    [[nodiscard]] const core::SemanticVersion& requested_version() const noexcept;
    [[nodiscard]] ToolImplementationStatus status() const noexcept;
    [[nodiscard]] bool is_bound() const noexcept;
    [[nodiscard]] const std::shared_ptr<const ToolTypeDefinition>& definition()
        const noexcept;
    [[nodiscard]] const std::shared_ptr<const ToolImplementation>& implementation() const noexcept;

private:
    core::ToolTypeId requested_id_;
    core::SemanticVersion requested_version_;
    ToolImplementationStatus status_;
    std::shared_ptr<const ToolTypeDefinition> definition_;
    std::shared_ptr<const ToolImplementation> implementation_;
};

// Maps the same stable tool type ID plus exact semantic version the catalog and the
// persisted workflow use onto the native implementation that executes it. Definition
// identity stays the single key across authoring, persistence, and execution.
class ToolImplementationRegistry final {
public:
    void register_implementation(std::shared_ptr<const ToolImplementation> implementation);

    [[nodiscard]] ToolImplementationBinding bind(
        const core::ToolTypeId& id,
        const core::SemanticVersion& version,
        const ToolRegistry& definitions) const;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool contains(
        const core::ToolTypeId& id,
        const core::SemanticVersion& version) const noexcept;

    // The catalog definitions this registry cannot execute yet, in catalog order.
    [[nodiscard]] std::vector<std::shared_ptr<const ToolTypeDefinition>> unimplemented(
        const ToolRegistry& definitions) const;

private:
    [[nodiscard]] static std::string key(
        const core::ToolTypeId& id,
        const core::SemanticVersion& version);

    std::map<std::string, std::shared_ptr<const ToolImplementation>> implementations_by_key_;
};

}  // namespace nvision::workflow
