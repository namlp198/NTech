#include "nvision/core/subworkflow.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nvision::core {

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
    PortKey exposed_key,
    const PortDirection direction,
    WorkflowNodeId node_id,
    PortKey node_port_key)
    : exposed_key_(std::move(exposed_key)),
      direction_(direction),
      node_id_(std::move(node_id)),
      node_port_key_(std::move(node_port_key)) {
    if (!is_valid(direction_)) {
        throw std::invalid_argument("subworkflow port mapping direction is unsupported");
    }
}

const PortKey& SubworkflowPortMapping::exposed_key() const noexcept {
    return exposed_key_;
}

PortDirection SubworkflowPortMapping::direction() const noexcept {
    return direction_;
}

const WorkflowNodeId& SubworkflowPortMapping::node_id() const noexcept {
    return node_id_;
}

const PortKey& SubworkflowPortMapping::node_port_key() const noexcept {
    return node_port_key_;
}

SubworkflowParameterPromotion::SubworkflowParameterPromotion(
    ParameterKey exposed_key,
    WorkflowNodeId node_id,
    ParameterKey node_parameter_key)
    : exposed_key_(std::move(exposed_key)),
      node_id_(std::move(node_id)),
      node_parameter_key_(std::move(node_parameter_key)) {}

const ParameterKey& SubworkflowParameterPromotion::exposed_key() const noexcept {
    return exposed_key_;
}

const WorkflowNodeId& SubworkflowParameterPromotion::node_id() const noexcept {
    return node_id_;
}

const ParameterKey& SubworkflowParameterPromotion::node_parameter_key() const noexcept {
    return node_parameter_key_;
}

SubworkflowDefinition::SubworkflowDefinition(
    ToolTypeDefinition interface_definition,
    std::vector<WorkflowNode> nodes,
    std::vector<SubworkflowPortMapping> port_mappings,
    std::vector<SubworkflowParameterPromotion> parameter_promotions)
    : interface_definition_(std::move(interface_definition)),
      nodes_(std::move(nodes)),
      port_mappings_(std::move(port_mappings)),
      parameter_promotions_(std::move(parameter_promotions)) {
    if (nodes_.empty()) {
        throw std::invalid_argument("subworkflow must contain at least one node");
    }

    for (std::size_t index = 0U; index < nodes_.size(); ++index) {
        const auto duplicate = std::find_if(
            nodes_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            nodes_.end(),
            [this, index](const WorkflowNode& candidate) {
                return candidate.id() == nodes_[index].id();
            });
        if (duplicate != nodes_.end()) {
            throw std::invalid_argument("subworkflow node IDs must be unique");
        }
        if (nodes_[index].tool_type_id() == interface_definition_.id()) {
            throw std::invalid_argument("subworkflow must not contain itself as a node");
        }
    }

    for (std::size_t index = 0U; index < port_mappings_.size(); ++index) {
        const auto& mapping = port_mappings_[index];
        if (interface_definition_.find_port(mapping.exposed_key(), mapping.direction()) ==
            nullptr) {
            throw std::invalid_argument("subworkflow port mapping targets an undeclared port");
        }
        if (find_node(mapping.node_id()) == nullptr) {
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
        if (find_node(promotion.node_id()) == nullptr) {
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

const std::vector<WorkflowNode>& SubworkflowDefinition::nodes() const noexcept {
    return nodes_;
}

const std::vector<SubworkflowPortMapping>& SubworkflowDefinition::port_mappings() const noexcept {
    return port_mappings_;
}

const std::vector<SubworkflowParameterPromotion>&
SubworkflowDefinition::parameter_promotions() const noexcept {
    return parameter_promotions_;
}

const WorkflowNode* SubworkflowDefinition::find_node(const WorkflowNodeId& id) const noexcept {
    const auto match = std::find_if(
        nodes_.begin(), nodes_.end(), [&id](const WorkflowNode& node) { return node.id() == id; });
    return match == nodes_.end() ? nullptr : &*match;
}

SubworkflowBinding::SubworkflowBinding(
    std::vector<WorkflowNodeBinding> nodes,
    std::vector<WorkflowDiagnostic> diagnostics)
    : nodes_(std::move(nodes)), diagnostics_(std::move(diagnostics)) {}

const std::vector<WorkflowNodeBinding>& SubworkflowBinding::nodes() const noexcept {
    return nodes_;
}

const std::vector<WorkflowDiagnostic>& SubworkflowBinding::diagnostics() const noexcept {
    return diagnostics_;
}

const WorkflowNodeBinding* SubworkflowBinding::find_node(
    const WorkflowNodeId& id) const noexcept {
    const auto match = std::find_if(
        nodes_.begin(), nodes_.end(), [&id](const WorkflowNodeBinding& binding) {
            return binding.node().id() == id;
        });
    return match == nodes_.end() ? nullptr : &*match;
}

bool SubworkflowBinding::is_fully_resolved() const noexcept {
    return std::all_of(nodes_.begin(), nodes_.end(), [](const WorkflowNodeBinding& binding) {
        return binding.is_resolved();
    });
}

bool SubworkflowBinding::has_errors() const noexcept {
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

SubworkflowBinding bind_subworkflow(
    const SubworkflowDefinition& definition,
    const ToolRegistry& registry) {
    std::vector<WorkflowNodeBinding> bindings;
    bindings.reserve(definition.nodes().size());
    for (const auto& node : definition.nodes()) {
        bindings.push_back(bind_workflow_node(node, registry));
    }

    const auto resolved_definition =
        [&bindings](const WorkflowNodeId& id) -> std::shared_ptr<const ToolTypeDefinition> {
        const auto match = std::find_if(
            bindings.begin(), bindings.end(), [&id](const WorkflowNodeBinding& binding) {
                return binding.node().id() == id;
            });
        return match == bindings.end() ? nullptr : match->definition();
    };

    std::vector<WorkflowDiagnostic> diagnostics;
    for (const auto& mapping : definition.port_mappings()) {
        const auto node_definition = resolved_definition(mapping.node_id());
        if (!node_definition) {
            // The node binding already reports why it is unresolved. Mapping checks
            // stay silent so that one missing plugin cannot bury the real cause.
            continue;
        }
        const auto* node_port =
            node_definition->find_port(mapping.node_port_key(), mapping.direction());
        if (node_port == nullptr) {
            diagnostics.push_back(WorkflowDiagnostic::port(
                ResultCode("MISSING_MAPPED_PORT"),
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
                ResultCode("PORT_TYPE_MISMATCH"),
                DiagnosticSeverity::error,
                "subworkflow port " + mapping.exposed_key().value() +
                    " and node port " + mapping.node_port_key().value() +
                    " declare different data types",
                mapping.node_id(),
                mapping.exposed_key()));
        }
    }

    for (const auto& promotion : definition.parameter_promotions()) {
        const auto node_definition = resolved_definition(promotion.node_id());
        if (!node_definition) {
            continue;
        }
        const auto* node_parameter =
            node_definition->find_parameter(promotion.node_parameter_key());
        if (node_parameter == nullptr) {
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                ResultCode("MISSING_PROMOTED_PARAMETER"),
                DiagnosticSeverity::error,
                "node parameter " + promotion.node_parameter_key().value() +
                    " is not declared by the bound tool version",
                promotion.node_id(),
                promotion.node_parameter_key()));
            continue;
        }
        const auto* exposed_parameter =
            definition.interface_definition().find_parameter(promotion.exposed_key());
        const bool kind_matches =
            exposed_parameter->type().kind() == node_parameter->type().kind();
        const bool default_accepted =
            !exposed_parameter->default_value().has_value() ||
            node_parameter->type().accepts(*exposed_parameter->default_value());
        if (!kind_matches || !default_accepted) {
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                ResultCode("PROMOTED_PARAMETER_TYPE_MISMATCH"),
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
                ResultCode("UNPROMOTED_PARAMETER"),
                DiagnosticSeverity::warning,
                "subworkflow parameter " + parameter.key().value() +
                    " is not promoted to any node parameter",
                parameter.key()));
        }
    }

    return SubworkflowBinding(std::move(bindings), std::move(diagnostics));
}

}  // namespace nvision::core
