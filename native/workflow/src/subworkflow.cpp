#include "nvision/workflow/subworkflow.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nvision::workflow {

namespace {

bool is_valid(const PortDirection direction) noexcept {
    return direction == PortDirection::input || direction == PortDirection::output;
}

bool same_target(const SubworkflowPortMapping& left, const SubworkflowPortMapping& right) noexcept {
    return left.node_id() == right.node_id() && left.node_port_key() == right.node_port_key() &&
           left.direction() == right.direction();
}

}  // namespace

SubworkflowPortMapping::SubworkflowPortMapping(
    core::PortKey exposed_key,
    const PortDirection direction,
    core::WorkflowNodeId node_id,
    core::PortKey node_port_key)
    : exposed_key_(std::move(exposed_key)),
      direction_(direction),
      node_id_(std::move(node_id)),
      node_port_key_(std::move(node_port_key)) {
    if (!is_valid(direction_)) {
        throw std::invalid_argument("subworkflow port mapping direction is unsupported");
    }
}

const core::PortKey& SubworkflowPortMapping::exposed_key() const noexcept {
    return exposed_key_;
}

PortDirection SubworkflowPortMapping::direction() const noexcept {
    return direction_;
}

const core::WorkflowNodeId& SubworkflowPortMapping::node_id() const noexcept {
    return node_id_;
}

const core::PortKey& SubworkflowPortMapping::node_port_key() const noexcept {
    return node_port_key_;
}

SubworkflowParameterPromotion::SubworkflowParameterPromotion(
    core::ParameterKey exposed_key,
    core::WorkflowNodeId node_id,
    core::ParameterKey node_parameter_key)
    : exposed_key_(std::move(exposed_key)),
      node_id_(std::move(node_id)),
      node_parameter_key_(std::move(node_parameter_key)) {}

const core::ParameterKey& SubworkflowParameterPromotion::exposed_key() const noexcept {
    return exposed_key_;
}

const core::WorkflowNodeId& SubworkflowParameterPromotion::node_id() const noexcept {
    return node_id_;
}

const core::ParameterKey& SubworkflowParameterPromotion::node_parameter_key() const noexcept {
    return node_parameter_key_;
}

SubworkflowDefinition::SubworkflowDefinition(
    ToolTypeDefinition interface_definition,
    WorkflowGraph graph,
    std::vector<SubworkflowPortMapping> port_mappings,
    std::vector<SubworkflowParameterPromotion> parameter_promotions)
    : interface_definition_(std::move(interface_definition)),
      graph_(std::move(graph)),
      port_mappings_(std::move(port_mappings)),
      parameter_promotions_(std::move(parameter_promotions)) {
    for (const auto& node : graph_.nodes()) {
        if (node.tool_type_id() == interface_definition_.id()) {
            throw std::invalid_argument("subworkflow must not contain itself as a node");
        }
    }

    for (std::size_t index = 0U; index < port_mappings_.size(); ++index) {
        const auto& mapping = port_mappings_[index];
        if (interface_definition_.find_port(mapping.exposed_key(), mapping.direction()) ==
            nullptr) {
            throw std::invalid_argument("subworkflow port mapping targets an undeclared port");
        }
        if (graph_.find_node(mapping.node_id()) == nullptr) {
            throw std::invalid_argument("subworkflow port mapping targets an unknown node");
        }
        const auto duplicate_target = std::find_if(
            port_mappings_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            port_mappings_.end(),
            [&mapping](const SubworkflowPortMapping& candidate) {
                return same_target(mapping, candidate);
            });
        if (duplicate_target != port_mappings_.end()) {
            throw std::invalid_argument("subworkflow node ports must be mapped at most once");
        }
    }

    for (const auto& port : interface_definition_.ports()) {
        const auto mapped = std::count_if(
            port_mappings_.begin(),
            port_mappings_.end(),
            [&port](const SubworkflowPortMapping& mapping) {
                return mapping.exposed_key() == port.key() &&
                       mapping.direction() == port.direction();
            });
        if (mapped == 0) {
            throw std::invalid_argument("every subworkflow port must be mapped to a node port");
        }
        // An exposed input may feed several internal ports; an exposed output is
        // produced by exactly one internal port.
        if (port.direction() == PortDirection::output && mapped > 1) {
            throw std::invalid_argument("subworkflow output ports must map to exactly one source");
        }
    }

    for (std::size_t index = 0U; index < parameter_promotions_.size(); ++index) {
        const auto& promotion = parameter_promotions_[index];
        if (interface_definition_.find_parameter(promotion.exposed_key()) == nullptr) {
            throw std::invalid_argument("subworkflow promotion targets an undeclared parameter");
        }
        if (graph_.find_node(promotion.node_id()) == nullptr) {
            throw std::invalid_argument("subworkflow promotion targets an unknown node");
        }
        const auto duplicate = std::find_if(
            parameter_promotions_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            parameter_promotions_.end(),
            [&promotion](const SubworkflowParameterPromotion& candidate) {
                return candidate.exposed_key() == promotion.exposed_key() ||
                       (candidate.node_id() == promotion.node_id() &&
                        candidate.node_parameter_key() == promotion.node_parameter_key());
            });
        if (duplicate != parameter_promotions_.end()) {
            throw std::invalid_argument("subworkflow parameter promotions must be one to one");
        }
    }
}

const ToolTypeDefinition& SubworkflowDefinition::interface_definition() const noexcept {
    return interface_definition_;
}

const WorkflowGraph& SubworkflowDefinition::graph() const noexcept {
    return graph_;
}

const std::vector<SubworkflowPortMapping>& SubworkflowDefinition::port_mappings() const noexcept {
    return port_mappings_;
}

const std::vector<SubworkflowParameterPromotion>&
SubworkflowDefinition::parameter_promotions() const noexcept {
    return parameter_promotions_;
}

WorkflowGraphBinding bind_subworkflow(
    const SubworkflowDefinition& definition,
    const ToolRegistry& registry) {
    std::vector<NodePortAddress> externally_fed_inputs;
    for (const auto& mapping : definition.port_mappings()) {
        if (mapping.direction() == PortDirection::input) {
            externally_fed_inputs.emplace_back(mapping.node_id(), mapping.node_port_key());
        }
    }

    const WorkflowGraphBinding graph_binding =
        bind_workflow_graph(definition.graph(), registry, externally_fed_inputs);

    const auto definition_of =
        [&graph_binding](
            const core::WorkflowNodeId& id) -> std::shared_ptr<const ToolTypeDefinition> {
        const auto* binding = graph_binding.find_node(id);
        return binding == nullptr ? nullptr : binding->definition();
    };

    std::vector<WorkflowDiagnostic> diagnostics = graph_binding.diagnostics();
    for (const auto& mapping : definition.port_mappings()) {
        const auto node_definition = definition_of(mapping.node_id());
        if (!node_definition) {
            // The node binding already reports why it is unresolved. Mapping checks
            // stay silent so that one missing plugin cannot bury the real cause.
            continue;
        }
        const auto* node_port =
            node_definition->find_port(mapping.node_port_key(), mapping.direction());
        if (node_port == nullptr) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("MISSING_MAPPED_PORT"),
                DiagnosticSeverity::error,
                "node port " + mapping.node_port_key().value() +
                    " is not declared by the bound tool version",
                mapping.node_id(),
                mapping.node_port_key()));
            continue;
        }
        const auto* exposed_port =
            definition.interface_definition().find_port(mapping.exposed_key(), mapping.direction());
        if (exposed_port->data_type() != node_port->data_type()) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                core::ResultCode("PORT_TYPE_MISMATCH"),
                DiagnosticSeverity::error,
                "subworkflow port " + mapping.exposed_key().value() +
                    " and node port " + mapping.node_port_key().value() +
                    " declare different data types",
                mapping.node_id(),
                mapping.exposed_key()));
        }
    }

    for (const auto& promotion : definition.parameter_promotions()) {
        const auto node_definition = definition_of(promotion.node_id());
        if (!node_definition) {
            continue;
        }
        const auto* node_parameter =
            node_definition->find_parameter(promotion.node_parameter_key());
        if (node_parameter == nullptr) {
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                core::ResultCode("MISSING_PROMOTED_PARAMETER"),
                DiagnosticSeverity::error,
                "node parameter " + promotion.node_parameter_key().value() +
                    " is not declared by the bound tool version",
                promotion.node_id(),
                promotion.node_parameter_key()));
            continue;
        }
        // Guaranteed non-null: the constructor refuses a promotion whose exposed key the
        // interface does not declare.
        const auto* exposed_parameter =
            definition.interface_definition().find_parameter(promotion.exposed_key());
        // The exposed schema is what a Recipe may set, so every value it admits has to
        // reach the internal parameter. Comparing kinds and one default would let an
        // exposed enum of {a, b} bind to an internal enum of {a} and fail on the first run
        // that chose b.
        const bool assignable =
            exposed_parameter->type().is_assignable_to(node_parameter->type());
        const bool default_accepted =
            !exposed_parameter->default_value().has_value() ||
            node_parameter->type().accepts(*exposed_parameter->default_value());
        if (!assignable || !default_accepted) {
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                core::ResultCode("PROMOTED_PARAMETER_TYPE_MISMATCH"),
                DiagnosticSeverity::error,
                "subworkflow parameter " + promotion.exposed_key().value() +
                    " is not compatible with node parameter " +
                    promotion.node_parameter_key().value(),
                promotion.node_id(),
                promotion.exposed_key()));
        }
    }

    for (const auto& parameter : definition.interface_definition().parameters()) {
        const bool promoted = std::any_of(
            definition.parameter_promotions().begin(),
            definition.parameter_promotions().end(),
            [&parameter](const SubworkflowParameterPromotion& promotion) {
                return promotion.exposed_key() == parameter.key();
            });
        if (!promoted) {
            // Legal for a parameter that only drives editor visibility, but reported
            // because an unpromoted value never reaches an internal node.
            diagnostics.push_back(WorkflowDiagnostic::interface_parameter(
                core::ResultCode("UNPROMOTED_PARAMETER"),
                DiagnosticSeverity::warning,
                "subworkflow parameter " + parameter.key().value() +
                    " is not promoted to any node parameter",
                parameter.key()));
        }
    }

    return WorkflowGraphBinding(graph_binding.nodes(), std::move(diagnostics));
}

}  // namespace nvision::workflow
