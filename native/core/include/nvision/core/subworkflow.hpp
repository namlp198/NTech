#pragma once

#include "nvision/core/identifier.hpp"
#include "nvision/core/tool_definition.hpp"
#include "nvision/core/tool_registry.hpp"
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
// versioned tool contract; the platform keeps the internal nodes generic instead of
// promoting an application pipeline to a built-in engine.
class SubworkflowDefinition final {
public:
    SubworkflowDefinition(
        ToolTypeDefinition interface_definition,
        std::vector<WorkflowNode> nodes,
        std::vector<SubworkflowPortMapping> port_mappings,
        std::vector<SubworkflowParameterPromotion> parameter_promotions = {});

    [[nodiscard]] const ToolTypeDefinition& interface_definition() const noexcept;
    [[nodiscard]] const std::vector<WorkflowNode>& nodes() const noexcept;
    [[nodiscard]] const std::vector<SubworkflowPortMapping>& port_mappings() const noexcept;
    [[nodiscard]] const std::vector<SubworkflowParameterPromotion>& parameter_promotions()
        const noexcept;
    [[nodiscard]] const WorkflowNode* find_node(const WorkflowNodeId& id) const noexcept;

private:
    ToolTypeDefinition interface_definition_;
    std::vector<WorkflowNode> nodes_;
    std::vector<SubworkflowPortMapping> port_mappings_;
    std::vector<SubworkflowParameterPromotion> parameter_promotions_;
};

// The result of binding a subworkflow against a Tool Registry. Node bindings keep the
// persisted configuration of every node, resolved or not.
class SubworkflowBinding final {
public:
    SubworkflowBinding(
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

[[nodiscard]] SubworkflowBinding bind_subworkflow(
    const SubworkflowDefinition& definition,
    const ToolRegistry& registry);

}  // namespace nvision::core
