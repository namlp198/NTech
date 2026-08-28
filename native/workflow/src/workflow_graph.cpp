#include "nvision/workflow/workflow_graph.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace nvision::workflow {

namespace {

bool feeds(
    const std::vector<NodePortAddress>& addresses,
    const core::WorkflowNodeId& node_id,
    const core::PortKey& port_key) {
    return std::any_of(
        addresses.begin(), addresses.end(), [&node_id, &port_key](const NodePortAddress& address) {
            return address.node_id() == node_id && address.port_key() == port_key;
        });
}

// Kahn's algorithm over node indices. A graph that cannot be fully ordered has a cycle.
bool has_cycle(
    const std::vector<WorkflowNode>& nodes,
    const std::vector<WorkflowConnection>& connections) {
    const auto index_of = [&nodes](const core::WorkflowNodeId& id) {
        const auto match = std::find_if(
            nodes.begin(), nodes.end(), [&id](const WorkflowNode& node) {
                return node.id() == id;
            });
        return static_cast<std::size_t>(std::distance(nodes.begin(), match));
    };

    std::vector<std::size_t> incoming(nodes.size(), 0U);
    for (const auto& connection : connections) {
        ++incoming[index_of(connection.target_node_id())];
    }

    std::vector<std::size_t> ready;
    for (std::size_t index = 0U; index < nodes.size(); ++index) {
        if (incoming[index] == 0U) {
            ready.push_back(index);
        }
    }

    std::size_t ordered = 0U;
    while (!ready.empty()) {
        const std::size_t current = ready.back();
        ready.pop_back();
        ++ordered;
        for (const auto& connection : connections) {
            if (index_of(connection.source_node_id()) != current) {
                continue;
            }
            const std::size_t target = index_of(connection.target_node_id());
            if (--incoming[target] == 0U) {
                ready.push_back(target);
            }
        }
    }
    return ordered != nodes.size();
}

}  // namespace

WorkflowConnection::WorkflowConnection(
    core::WorkflowNodeId source_node_id,
    core::PortKey source_port_key,
    core::WorkflowNodeId target_node_id,
    core::PortKey target_port_key)
    : source_node_id_(std::move(source_node_id)),
      source_port_key_(std::move(source_port_key)),
      target_node_id_(std::move(target_node_id)),
      target_port_key_(std::move(target_port_key)) {
    if (source_node_id_ == target_node_id_) {
        throw std::invalid_argument("a workflow connection must not start and end on one node");
    }
}

const core::WorkflowNodeId& WorkflowConnection::source_node_id() const noexcept {
    return source_node_id_;
}

const core::PortKey& WorkflowConnection::source_port_key() const noexcept {
    return source_port_key_;
}

const core::WorkflowNodeId& WorkflowConnection::target_node_id() const noexcept {
    return target_node_id_;
}

const core::PortKey& WorkflowConnection::target_port_key() const noexcept {
    return target_port_key_;
}

NodePortAddress::NodePortAddress(core::WorkflowNodeId node_id, core::PortKey port_key)
    : node_id_(std::move(node_id)), port_key_(std::move(port_key)) {}

const core::WorkflowNodeId& NodePortAddress::node_id() const noexcept {
    return node_id_;
}

const core::PortKey& NodePortAddress::port_key() const noexcept {
    return port_key_;
}

WorkflowGraph::WorkflowGraph(
    std::vector<WorkflowNode> nodes,
    std::vector<WorkflowConnection> connections)
    : nodes_(std::move(nodes)), connections_(std::move(connections)) {
    if (nodes_.empty()) {
        throw std::invalid_argument("a workflow graph must contain at least one node");
    }

    for (std::size_t index = 0U; index < nodes_.size(); ++index) {
        const auto duplicate = std::find_if(
            nodes_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            nodes_.end(),
            [this, index](const WorkflowNode& candidate) {
                return candidate.id() == nodes_[index].id();
            });
        if (duplicate != nodes_.end()) {
            throw std::invalid_argument("workflow node IDs must be unique");
        }
    }

    for (std::size_t index = 0U; index < connections_.size(); ++index) {
        const auto& connection = connections_[index];
        if (find_node(connection.source_node_id()) == nullptr ||
            find_node(connection.target_node_id()) == nullptr) {
            throw std::invalid_argument("a workflow connection references an unknown node");
        }
        const auto duplicate = std::find(
            connections_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            connections_.end(),
            connection);
        if (duplicate != connections_.end()) {
            throw std::invalid_argument("workflow connections must be unique");
        }
    }

    if (has_cycle(nodes_, connections_)) {
        throw std::invalid_argument("a workflow graph must not contain a cycle");
    }
}

const std::vector<WorkflowNode>& WorkflowGraph::nodes() const noexcept {
    return nodes_;
}

const std::vector<WorkflowConnection>& WorkflowGraph::connections() const noexcept {
    return connections_;
}

const WorkflowNode* WorkflowGraph::find_node(const core::WorkflowNodeId& id) const noexcept {
    const auto match = std::find_if(
        nodes_.begin(), nodes_.end(), [&id](const WorkflowNode& node) { return node.id() == id; });
    return match == nodes_.end() ? nullptr : &*match;
}

WorkflowGraphBinding::WorkflowGraphBinding(
    std::vector<WorkflowNodeBinding> nodes,
    std::vector<WorkflowDiagnostic> diagnostics)
    : nodes_(std::move(nodes)), diagnostics_(std::move(diagnostics)) {}

const std::vector<WorkflowNodeBinding>& WorkflowGraphBinding::nodes() const noexcept {
    return nodes_;
}

const std::vector<WorkflowDiagnostic>& WorkflowGraphBinding::diagnostics() const noexcept {
    return diagnostics_;
}

const WorkflowNodeBinding* WorkflowGraphBinding::find_node(
    const core::WorkflowNodeId& id) const noexcept {
    const auto match = std::find_if(
        nodes_.begin(), nodes_.end(), [&id](const WorkflowNodeBinding& binding) {
            return binding.node().id() == id;
        });
    return match == nodes_.end() ? nullptr : &*match;
}

bool WorkflowGraphBinding::is_fully_resolved() const noexcept {
    return std::all_of(nodes_.begin(), nodes_.end(), [](const WorkflowNodeBinding& binding) {
        return binding.is_resolved();
    });
}

bool WorkflowGraphBinding::has_errors() const noexcept {
    const bool node_errors =
        std::any_of(nodes_.begin(), nodes_.end(), [](const WorkflowNodeBinding& binding) {
            return binding.has_errors();
        });
    return node_errors ||
           std::any_of(
               diagnostics_.begin(),
               diagnostics_.end(),
               [](const WorkflowDiagnostic& diagnostic) {
                   return diagnostic.severity() == DiagnosticSeverity::error;
               });
}

WorkflowGraphBinding bind_workflow_graph(
    const WorkflowGraph& graph,
    const ToolRegistry& registry,
    const std::vector<NodePortAddress>& externally_fed_inputs) {
    std::vector<WorkflowNodeBinding> bindings;
    bindings.reserve(graph.nodes().size());
    for (const auto& node : graph.nodes()) {
        bindings.push_back(bind_workflow_node(node, registry));
    }

    const auto definition_of =
        [&bindings](const core::WorkflowNodeId& id) -> std::shared_ptr<const ToolTypeDefinition> {
        const auto match = std::find_if(
            bindings.begin(), bindings.end(), [&id](const WorkflowNodeBinding& binding) {
                return binding.node().id() == id;
            });
        return match == bindings.end() ? nullptr : match->definition();
    };

    std::vector<WorkflowDiagnostic> diagnostics;
    std::vector<NodePortAddress> connected_inputs;
    for (const auto& connection : graph.connections()) {
        // Recorded before resolution is considered so that an unresolved source cannot
        // make a connected input look unconnected.
        const bool already_fed = feeds(
            connected_inputs, connection.target_node_id(), connection.target_port_key());
        connected_inputs.emplace_back(connection.target_node_id(), connection.target_port_key());

        const auto source = definition_of(connection.source_node_id());
        const auto target = definition_of(connection.target_node_id());
        if (!source || !target) {
            // The unresolved endpoint already reports why. Staying silent here keeps one
            // missing plugin from producing a diagnostic per edge that touches it.
            continue;
        }

        const auto* source_port =
            source->find_port(connection.source_port_key(), PortDirection::output);
        if (source_port == nullptr) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("MISSING_CONNECTED_PORT"),
                DiagnosticSeverity::error,
                "output port " + connection.source_port_key().value() +
                    " is not declared by the bound tool version",
                connection.source_node_id(),
                connection.source_port_key()));
            continue;
        }

        const auto* target_port =
            target->find_port(connection.target_port_key(), PortDirection::input);
        if (target_port == nullptr) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("MISSING_CONNECTED_PORT"),
                DiagnosticSeverity::error,
                "input port " + connection.target_port_key().value() +
                    " is not declared by the bound tool version",
                connection.target_node_id(),
                connection.target_port_key()));
            continue;
        }

        if (source_port->data_type() != target_port->data_type()) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("CONNECTION_TYPE_MISMATCH"),
                DiagnosticSeverity::error,
                "output port " + connection.source_port_key().value() + " of node " +
                    connection.source_node_id().value() + " does not match input port " +
                    connection.target_port_key().value(),
                connection.target_node_id(),
                connection.target_port_key()));
        }

        if (already_fed && !target_port->allows_multiple()) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("PORT_ALREADY_CONNECTED"),
                DiagnosticSeverity::error,
                "input port " + connection.target_port_key().value() +
                    " accepts a single source",
                connection.target_node_id(),
                connection.target_port_key()));
        }
    }

    for (const auto& binding : bindings) {
        if (!binding.is_resolved()) {
            continue;
        }
        for (const auto& port : binding.definition()->ports()) {
            if (port.direction() != PortDirection::input || !port.required()) {
                continue;
            }
            if (feeds(connected_inputs, binding.node().id(), port.key()) ||
                feeds(externally_fed_inputs, binding.node().id(), port.key())) {
                continue;
            }
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("MISSING_REQUIRED_INPUT"),
                DiagnosticSeverity::error,
                "required input port " + port.key().value() + " has no source",
                binding.node().id(),
                port.key()));
        }
    }

    return WorkflowGraphBinding(std::move(bindings), std::move(diagnostics));
}

}  // namespace nvision::workflow
