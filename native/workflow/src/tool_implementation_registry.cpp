#include "nvision/workflow/tool_implementation_registry.hpp"

#include <stdexcept>
#include <utility>

namespace nvision::workflow {

namespace {

bool is_valid(const ToolImplementationStatus status) noexcept {
    return status == ToolImplementationStatus::bound ||
           status == ToolImplementationStatus::unknown_tool ||
           status == ToolImplementationStatus::unsupported_version ||
           status == ToolImplementationStatus::not_implemented;
}

}  // namespace

ToolImplementationBinding::ToolImplementationBinding(
    core::ToolTypeId requested_id,
    core::SemanticVersion requested_version,
    const ToolImplementationStatus status,
    std::shared_ptr<const core::ToolTypeDefinition> definition,
    std::shared_ptr<const ToolImplementation> implementation)
    : requested_id_(std::move(requested_id)),
      requested_version_(std::move(requested_version)),
      status_(status),
      definition_(std::move(definition)),
      implementation_(std::move(implementation)) {
    if (!is_valid(status_)) {
        throw std::invalid_argument("tool implementation status is unsupported");
    }
    if ((status_ == ToolImplementationStatus::bound) != static_cast<bool>(implementation_)) {
        throw std::invalid_argument("bound implementation status and implementation must agree");
    }
    // A catalog definition is required for every status that presumes the tool exists.
    const bool expects_definition = status_ == ToolImplementationStatus::bound ||
                                    status_ == ToolImplementationStatus::not_implemented;
    if (expects_definition != static_cast<bool>(definition_)) {
        throw std::invalid_argument("implementation binding status and definition must agree");
    }
    if (definition_ && (definition_->id() != requested_id_ ||
                        definition_->version() != requested_version_)) {
        throw std::invalid_argument("bound tool definition must match the requested identity");
    }
}

const core::ToolTypeId& ToolImplementationBinding::requested_id() const noexcept {
    return requested_id_;
}

const core::SemanticVersion& ToolImplementationBinding::requested_version() const noexcept {
    return requested_version_;
}

ToolImplementationStatus ToolImplementationBinding::status() const noexcept {
    return status_;
}

bool ToolImplementationBinding::is_bound() const noexcept {
    return status_ == ToolImplementationStatus::bound;
}

const std::shared_ptr<const core::ToolTypeDefinition>& ToolImplementationBinding::definition()
    const noexcept {
    return definition_;
}

const std::shared_ptr<const ToolImplementation>& ToolImplementationBinding::implementation()
    const noexcept {
    return implementation_;
}

void ToolImplementationRegistry::register_implementation(
    std::shared_ptr<const ToolImplementation> implementation) {
    if (!implementation) {
        throw std::invalid_argument("a tool implementation must not be null");
    }
    std::string implementation_key =
        key(implementation->tool_type_id(), implementation->tool_version());
    if (implementations_by_key_.contains(implementation_key)) {
        throw std::invalid_argument(
            "tool type ID and version already have a registered implementation");
    }
    implementations_by_key_.emplace(std::move(implementation_key), std::move(implementation));
}

ToolImplementationBinding ToolImplementationRegistry::bind(
    const core::ToolTypeId& id,
    const core::SemanticVersion& version,
    const core::ToolRegistry& definitions) const {
    // Definition resolution comes first so a workflow sees the same unknown-tool and
    // unsupported-version states at execution time that authoring already reported.
    const core::ToolResolution resolution = definitions.resolve_exact(id, version);
    if (!resolution.is_resolved()) {
        const bool unknown = resolution.status() == core::ToolResolutionStatus::unknown_tool;
        return ToolImplementationBinding(
            id,
            version,
            unknown ? ToolImplementationStatus::unknown_tool
                    : ToolImplementationStatus::unsupported_version,
            nullptr,
            nullptr);
    }

    const auto match = implementations_by_key_.find(key(id, version));
    if (match == implementations_by_key_.end()) {
        return ToolImplementationBinding(
            id,
            version,
            ToolImplementationStatus::not_implemented,
            resolution.definition(),
            nullptr);
    }
    return ToolImplementationBinding(
        id, version, ToolImplementationStatus::bound, resolution.definition(), match->second);
}

std::size_t ToolImplementationRegistry::size() const noexcept {
    return implementations_by_key_.size();
}

bool ToolImplementationRegistry::contains(
    const core::ToolTypeId& id,
    const core::SemanticVersion& version) const noexcept {
    return implementations_by_key_.contains(key(id, version));
}

std::vector<std::shared_ptr<const core::ToolTypeDefinition>>
ToolImplementationRegistry::unimplemented(const core::ToolRegistry& definitions) const {
    std::vector<std::shared_ptr<const core::ToolTypeDefinition>> pending;
    for (const auto& definition : definitions.definitions()) {
        if (!contains(definition->id(), definition->version())) {
            pending.push_back(definition);
        }
    }
    return pending;
}

std::string ToolImplementationRegistry::key(
    const core::ToolTypeId& id,
    const core::SemanticVersion& version) {
    return id.value() + "@" + version.to_string();
}

}  // namespace nvision::workflow
