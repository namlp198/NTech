#pragma once

#include "nvision/core/identifier.hpp"
#include "nvision/core/tool_definition.hpp"
#include "nvision/core/tool_registry.hpp"
#include "nvision/core/workflow_graph.hpp"
#include "nvision/core/workflow_node.hpp"

#include <vector>

namespace nvision::core {

// Binds one port of the subworkflow interface to a port of one internal node.
// Direction is carried explicitly because a tool may declare the same port key
// once per direction.
class SubworkflowPortMapping final {
public:
    SubworkflowPortMapping(
        PortKey exposed_key,
        PortDirection direction,
        WorkflowNodeId node_id,
        PortKey node_port_key);

    [[nodiscard]] const PortKey& exposed_key() const noexcept;
    [[nodiscard]] PortDirection direction() const noexcept;
    [[nodiscard]] const WorkflowNodeId& node_id() const noexcept;
    [[nodiscard]] const PortKey& node_port_key() const noexcept;

private:
    PortKey exposed_key_;
    PortDirection direction_;
    WorkflowNodeId node_id_;
    PortKey node_port_key_;
};

// Forwards one interface parameter to the parameter of one internal node.
class SubworkflowParameterPromotion final {
public:
    SubworkflowParameterPromotion(
        ParameterKey exposed_key,
        WorkflowNodeId node_id,
        ParameterKey node_parameter_key);

    [[nodiscard]] const ParameterKey& exposed_key() const noexcept;
    [[nodiscard]] const WorkflowNodeId& node_id() const noexcept;
    [[nodiscard]] const ParameterKey& node_parameter_key() const noexcept;

private:
    ParameterKey exposed_key_;
    WorkflowNodeId node_id_;
    ParameterKey node_parameter_key_;
};

// A reusable graph published through an ordinary tool interface. The catalog sees a
// versioned tool contract; the platform keeps the internal graph generic instead of
// promoting an application pipeline to a built-in engine.
class SubworkflowDefinition final {
public:
    SubworkflowDefinition(
        ToolTypeDefinition interface_definition,
        WorkflowGraph graph,
        std::vector<SubworkflowPortMapping> port_mappings,
        std::vector<SubworkflowParameterPromotion> parameter_promotions = {});

    [[nodiscard]] const ToolTypeDefinition& interface_definition() const noexcept;
    [[nodiscard]] const WorkflowGraph& graph() const noexcept;
    [[nodiscard]] const std::vector<SubworkflowPortMapping>& port_mappings() const noexcept;
    [[nodiscard]] const std::vector<SubworkflowParameterPromotion>& parameter_promotions()
        const noexcept;

private:
    ToolTypeDefinition interface_definition_;
    WorkflowGraph graph_;
    std::vector<SubworkflowPortMapping> port_mappings_;
    std::vector<SubworkflowParameterPromotion> parameter_promotions_;
};

// Binds the internal graph and reports interface mapping and promotion problems in the
// same diagnostic list. Node bindings keep the persisted configuration of every node,
// resolved or not.
[[nodiscard]] WorkflowGraphBinding bind_subworkflow(
    const SubworkflowDefinition& definition,
    const ToolRegistry& registry);

}  // namespace nvision::core
