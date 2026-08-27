#include "nvision/workflow/tool_registry.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nvision::workflow {

namespace {

bool is_valid(const ToolResolutionStatus status) noexcept {
    return status == ToolResolutionStatus::resolved ||
           status == ToolResolutionStatus::unknown_tool ||
           status == ToolResolutionStatus::unsupported_version;
}

}  // namespace

ToolResolution::ToolResolution(
    core::ToolTypeId requested_id,
    core::SemanticVersion requested_version,
    const ToolResolutionStatus status,
    std::shared_ptr<const ToolTypeDefinition> definition)
    : requested_id_(std::move(requested_id)),
      requested_version_(std::move(requested_version)),
      status_(status),
      definition_(std::move(definition)) {
    if (!is_valid(status_)) {
        throw std::invalid_argument("tool resolution status is unsupported");
    }
    if ((status_ == ToolResolutionStatus::resolved) != static_cast<bool>(definition_)) {
        throw std::invalid_argument("resolved tool status and definition must agree");
    }
    if (definition_ &&
        (definition_->id() != requested_id_ || definition_->version() != requested_version_)) {
        throw std::invalid_argument("resolved tool definition must match the requested identity");
    }
}

const core::ToolTypeId& ToolResolution::requested_id() const noexcept {
    return requested_id_;
}

const core::SemanticVersion& ToolResolution::requested_version() const noexcept {
    return requested_version_;
}

ToolResolutionStatus ToolResolution::status() const noexcept {
    return status_;
}

bool ToolResolution::is_resolved() const noexcept {
    return status_ == ToolResolutionStatus::resolved;
}

const std::shared_ptr<const ToolTypeDefinition>& ToolResolution::definition() const noexcept {
    return definition_;
}

std::shared_ptr<const ToolTypeDefinition> ToolRegistry::register_definition(
    ToolTypeDefinition definition) {
    const std::string definition_key = key(definition.id(), definition.version());
    if (definitions_by_key_.contains(definition_key)) {
        throw std::invalid_argument("tool type ID and version are already registered");
    }

    auto registered = std::make_shared<const ToolTypeDefinition>(std::move(definition));
    definitions_by_key_.emplace(definition_key, registered);
    definitions_.push_back(registered);
    return registered;
}

ToolResolution ToolRegistry::resolve_exact(
    const core::ToolTypeId& id,
    const core::SemanticVersion& version) const {
    const auto match = definitions_by_key_.find(key(id, version));
    if (match != definitions_by_key_.end()) {
        return ToolResolution(id, version, ToolResolutionStatus::resolved, match->second);
    }

    const bool known_id = std::any_of(
        definitions_.begin(), definitions_.end(), [&id](const auto& definition) {
            return definition->id() == id;
        });
    return ToolResolution(
        id,
        version,
        known_id ? ToolResolutionStatus::unsupported_version
                 : ToolResolutionStatus::unknown_tool);
}

const std::vector<std::shared_ptr<const ToolTypeDefinition>>& ToolRegistry::definitions()
    const noexcept {
    return definitions_;
}

std::vector<core::SemanticVersion> ToolRegistry::available_versions(const core::ToolTypeId& id) const {
    std::vector<core::SemanticVersion> versions;
    for (const auto& definition : definitions_) {
        if (definition->id() == id) {
            versions.push_back(definition->version());
        }
    }
    std::sort(versions.begin(), versions.end(), [](const auto& left, const auto& right) {
        const int precedence = left.compare_precedence(right);
        return precedence == 0 ? left.to_string() < right.to_string() : precedence < 0;
    });
    return versions;
}

std::string ToolRegistry::key(const core::ToolTypeId& id, const core::SemanticVersion& version) {
    return id.value() + "@" + version.to_string();
}

}  // namespace nvision::workflow
