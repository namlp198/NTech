#pragma once

#include "nvision/core/execution_result.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/parameter_schema.hpp"
#include "nvision/core/semantic_version.hpp"
#include "nvision/core/tool_definition.hpp"
#include "nvision/core/tool_registry.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nvision::core {

class NodeParameterAssignment final {
public:
    NodeParameterAssignment(ParameterKey key, ParameterValue value);

    [[nodiscard]] const ParameterKey& key() const noexcept;
    [[nodiscard]] const ParameterValue& value() const noexcept;

private:
    ParameterKey key_;
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
    [[nodiscard]] const ParameterValue* find(const ParameterKey& key) const noexcept;
    [[nodiscard]] bool contains(const ParameterKey& key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::vector<NodeParameterAssignment> assignments_;
};

// A configured tool instance inside a workflow. Identity is the stable tool type ID
// plus its exact semantic version; catalog display structure is never persisted here.
class WorkflowNode final {
public:
    WorkflowNode(
        WorkflowNodeId id,
        ToolTypeId tool_type_id,
        SemanticVersion tool_version,
        std::string display_name,
        NodeConfiguration configuration = {});

    [[nodiscard]] const WorkflowNodeId& id() const noexcept;
    [[nodiscard]] const ToolTypeId& tool_type_id() const noexcept;
    [[nodiscard]] const SemanticVersion& tool_version() const noexcept;
    [[nodiscard]] const std::string& display_name() const noexcept;
    [[nodiscard]] const NodeConfiguration& configuration() const noexcept;

private:
    WorkflowNodeId id_;
    ToolTypeId tool_type_id_;
    SemanticVersion tool_version_;
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
        ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        WorkflowNodeId node_id);
    [[nodiscard]] static WorkflowDiagnostic parameter(
        ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        WorkflowNodeId node_id,
        ParameterKey parameter_key);
    [[nodiscard]] static WorkflowDiagnostic port(
        ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        WorkflowNodeId node_id,
        PortKey port_key);
    // Scoped to a composed contract rather than to one node.
    [[nodiscard]] static WorkflowDiagnostic interface_parameter(
        ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        ParameterKey parameter_key);

    [[nodiscard]] const ResultCode& code() const noexcept;
    [[nodiscard]] DiagnosticSeverity severity() const noexcept;
    [[nodiscard]] const std::string& detail() const noexcept;
    [[nodiscard]] const std::optional<WorkflowNodeId>& node_id() const noexcept;
    [[nodiscard]] const std::optional<ParameterKey>& parameter_key() const noexcept;
    [[nodiscard]] const std::optional<PortKey>& port_key() const noexcept;

private:
    WorkflowDiagnostic(
        ResultCode code,
        DiagnosticSeverity severity,
        std::string detail,
        std::optional<WorkflowNodeId> node_id,
        std::optional<ParameterKey> parameter_key,
        std::optional<PortKey> port_key);

    ResultCode code_;
    DiagnosticSeverity severity_;
    std::string detail_;
    std::optional<WorkflowNodeId> node_id_;
    std::optional<ParameterKey> parameter_key_;
    std::optional<PortKey> port_key_;
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

}  // namespace nvision::core
