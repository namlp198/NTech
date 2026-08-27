#pragma once

#include "nvision/core/execution_result.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/semantic_version.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/tool_definition.hpp"
#include "nvision/workflow/tool_registry.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nvision::workflow {

class NodeParameterAssignment final {
public:
    NodeParameterAssignment(core::ParameterKey key, ParameterValue value);

    [[nodiscard]] const core::ParameterKey& key() const noexcept;
    [[nodiscard]] const ParameterValue& value() const noexcept;

private:
    core::ParameterKey key_;
    ParameterValue value_;
};

// Persisted parameter configuration of one workflow node. Assignments keep their
// authored order and are never reordered, coerced, or dropped, including keys the
// resolved tool version does not declare.
class NodeConfiguration final {
public:
    NodeConfiguration() = default;
    explicit NodeConfiguration(std::vector<NodeParameterAssignment> assignments);

    [[nodiscard]] const std::vector<NodeParameterAssignment>& assignments() const noexcept;
    [[nodiscard]] const ParameterValue* find(const core::ParameterKey& key) const noexcept;
    [[nodiscard]] bool contains(const core::ParameterKey& key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::vector<NodeParameterAssignment> assignments_;
};

// A configured tool instance inside a workflow. Identity is the stable tool type ID
// plus its exact semantic version; catalog display structure is never persisted here.
class WorkflowNode final {
public:
    WorkflowNode(
        core::WorkflowNodeId id,
        core::ToolTypeId tool_type_id,
        core::SemanticVersion tool_version,
        std::string display_name,
        NodeConfiguration configuration = {});

    [[nodiscard]] const core::WorkflowNodeId& id() const noexcept;
    [[nodiscard]] const core::ToolTypeId& tool_type_id() const noexcept;
    [[nodiscard]] const core::SemanticVersion& tool_version() const noexcept;
    [[nodiscard]] const std::string& display_name() const noexcept;
    [[nodiscard]] const NodeConfiguration& configuration() const noexcept;

private:
    core::WorkflowNodeId id_;
    core::ToolTypeId tool_type_id_;
    core::SemanticVersion tool_version_;
    std::string display_name_;
    NodeConfiguration configuration_;
};

enum class DiagnosticSeverity : std::uint32_t {
    warning = 0,
    error = 1,
};

class WorkflowDiagnostic final {
public:
    [[nodiscard]] static WorkflowDiagnostic node(
        core::ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        core::WorkflowNodeId node_id);
    [[nodiscard]] static WorkflowDiagnostic parameter(
        core::ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        core::WorkflowNodeId node_id,
        core::ParameterKey parameter_key);
    [[nodiscard]] static WorkflowDiagnostic port(
        core::ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        core::WorkflowNodeId node_id,
        core::PortKey port_key);
    // Scoped to a composed contract rather than to one node.
    [[nodiscard]] static WorkflowDiagnostic interface_parameter(
        core::ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        core::ParameterKey parameter_key);

    [[nodiscard]] const core::ResultCode& code() const noexcept;
    [[nodiscard]] DiagnosticSeverity severity() const noexcept;
    [[nodiscard]] const std::string& detail() const noexcept;
    [[nodiscard]] const std::optional<core::WorkflowNodeId>& node_id() const noexcept;
    [[nodiscard]] const std::optional<core::ParameterKey>& parameter_key() const noexcept;
    [[nodiscard]] const std::optional<core::PortKey>& port_key() const noexcept;

private:
    WorkflowDiagnostic(
        core::ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        std::optional<core::WorkflowNodeId> node_id,
        std::optional<core::ParameterKey> parameter_key,
        std::optional<core::PortKey> port_key);

    core::ResultCode code_;
    DiagnosticSeverity severity_;
    std::string detail_;
    std::optional<core::WorkflowNodeId> node_id_;
    std::optional<core::ParameterKey> parameter_key_;
    std::optional<core::PortKey> port_key_;
};

// The result of binding one persisted node against a Tool Registry. An unresolved
// node keeps its authored identity and configuration so that a missing plugin or an
// unsupported tool version can never silently rewrite the stored workflow.
class WorkflowNodeBinding final {
public:
    WorkflowNodeBinding(
        WorkflowNode node,
        ToolResolutionStatus status,
        std::shared_ptr<const ToolTypeDefinition> definition,
        std::vector<WorkflowDiagnostic> diagnostics);

    [[nodiscard]] const WorkflowNode& node() const noexcept;
    [[nodiscard]] ToolResolutionStatus status() const noexcept;
    [[nodiscard]] bool is_resolved() const noexcept;
    [[nodiscard]] const std::shared_ptr<const ToolTypeDefinition>& definition() const noexcept;
    [[nodiscard]] const std::vector<WorkflowDiagnostic>& diagnostics() const noexcept;
    [[nodiscard]] bool has_errors() const noexcept;

private:
    WorkflowNode node_;
    ToolResolutionStatus status_;
    std::shared_ptr<const ToolTypeDefinition> definition_;
    std::vector<WorkflowDiagnostic> diagnostics_;
};

[[nodiscard]] WorkflowNodeBinding bind_workflow_node(
    WorkflowNode node,
    const ToolRegistry& registry);

}  // namespace nvision::workflow
