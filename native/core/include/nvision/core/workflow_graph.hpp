#pragma once

#include "nvision/core/identifier.hpp"
#include "nvision/core/tool_registry.hpp"
#include "nvision/core/workflow_node.hpp"

#include <vector>

namespace nvision::core {

// One typed edge from an output port of a node to an input port of another node.
class WorkflowConnection final {
public:
    WorkflowConnection(
        WorkflowNodeId source_node_id,
        PortKey source_port_key,
        WorkflowNodeId target_node_id,
        PortKey target_port_key);

    [[nodiscard]] const WorkflowNodeId& source_node_id() const noexcept;
    [[nodiscard]] const PortKey& source_port_key() const noexcept;
    [[nodiscard]] const WorkflowNodeId& target_node_id() const noexcept;
    [[nodiscard]] const PortKey& target_port_key() const noexcept;
    [[nodiscard]] bool operator==(const WorkflowConnection& other) const noexcept = default;

private:
    WorkflowNodeId source_node_id_;
    PortKey source_port_key_;
    WorkflowNodeId target_node_id_;
    PortKey target_port_key_;
};

// Addresses one port of one node. Used to declare inputs that are fed from outside the
// graph, for example by a subworkflow interface mapping or by a Job binding.
class NodePortAddress final {
public:
    NodePortAddress(WorkflowNodeId node_id, PortKey port_key);

    [[nodiscard]] const WorkflowNodeId& node_id() const noexcept;
    [[nodiscard]] const PortKey& port_key() const noexcept;
    [[nodiscard]] bool operator==(const NodePortAddress& other) const noexcept = default;

private:
    WorkflowNodeId node_id_;
    PortKey port_key_;
};

// A directed acyclic graph of configured tool instances. Topology is validated without a
// registry; everything that depends on a tool contract is validated when the graph binds.
class WorkflowGraph final {
public:
    WorkflowGraph(std::vector<WorkflowNode> nodes, std::vector<WorkflowConnection> connections);

    [[nodiscard]] const std::vector<WorkflowNode>& nodes() const noexcept;
    [[nodiscard]] const std::vector<WorkflowConnection>& connections() const noexcept;
    [[nodiscard]] const WorkflowNode* find_node(const WorkflowNodeId& id) const noexcept;

private:
    std::vector<WorkflowNode> nodes_;
    std::vector<WorkflowConnection> connections_;
};

// The result of binding every node and connection of a graph against a Tool Registry.
class WorkflowGraphBinding final {
public:
    WorkflowGraphBinding(
        std::vector<WorkflowNodeBinding> nodes,
        std::vector<WorkflowDiagnostic> diagnostics);

    [[nodiscard]] const std::vector<WorkflowNodeBinding>& nodes() const noexcept;
    [[nodiscard]] const std::vector<WorkflowDiagnostic>& diagnostics() const noexcept;
    [[nodiscard]] const WorkflowNodeBinding* find_node(const WorkflowNodeId& id) const noexcept;
    [[nodiscard]] bool is_fully_resolved() const noexcept;
    [[nodiscard]] bool has_errors() const noexcept;

private:
    std::vector<WorkflowNodeBinding> nodes_;
    std::vector<WorkflowDiagnostic> diagnostics_;
};

// `externally_fed_inputs` names input ports that a caller supplies, so that a required
// input is not reported as unconnected when the graph is embedded in a larger contract.
[[nodiscard]] WorkflowGraphBinding bind_workflow_graph(
    const WorkflowGraph& graph,
    const ToolRegistry& registry,
    const std::vector<NodePortAddress>& externally_fed_inputs = {});

}  // namespace nvision::core
