using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class PortValidationStage : IWorkflowValidationStage
{
    public WorkflowValidationStage Stage => WorkflowValidationStage.Port;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        foreach (WorkflowNode node in context.Workflow.Nodes)
        {
            var resolution = context.Registry.Resolve(node.NodeTypeId, node.NodeTypeVersion);
            if (resolution.IsFailure)
            {
                continue;
            }

            NodeTypeDefinition definition = resolution.Value;
            foreach (ValidationIssue issue in ValidateContract(node, definition))
            {
                yield return issue;
            }

            if (!node.Enabled)
            {
                continue;
            }

            foreach (PortDefinition requiredInput in definition.InputPorts.Where(port => port.Required))
            {
                bool connected = context.Workflow.Connections
                    .Where(connection => connection.TargetNodeId == node.Id && connection.TargetPort == requiredInput.Key)
                    .Any(connection => context.Workflow.Nodes.Single(source => source.Id == connection.SourceNodeId).Enabled);
                if (!connected)
                {
                    yield return WorkflowValidationIssueFactory.Error(
                        "WFL-006",
                        $"Required input port '{requiredInput.Key}' is not connected to an enabled source.",
                        $"nodes/{node.Id}/ports/{requiredInput.Key}");
                }
            }
        }

        foreach (WorkflowConnection connection in context.Workflow.Connections)
        {
            WorkflowNode sourceNode = context.Workflow.Nodes.Single(node => node.Id == connection.SourceNodeId);
            WorkflowNode targetNode = context.Workflow.Nodes.Single(node => node.Id == connection.TargetNodeId);
            var sourceResolution = context.Registry.Resolve(sourceNode.NodeTypeId, sourceNode.NodeTypeVersion);
            var targetResolution = context.Registry.Resolve(targetNode.NodeTypeId, targetNode.NodeTypeVersion);
            if (sourceResolution.IsFailure || targetResolution.IsFailure)
            {
                continue;
            }

            PortDefinition? sourcePort = sourceResolution.Value.FindPort(connection.SourcePort, PortDirection.Output);
            PortDefinition? targetPort = targetResolution.Value.FindPort(connection.TargetPort, PortDirection.Input);
            if (sourcePort is not null && targetPort is not null && sourcePort.DataType != targetPort.DataType)
            {
                yield return WorkflowValidationIssueFactory.Error(
                    "WFL-005",
                    $"Port data types '{sourcePort.DataType}' and '{targetPort.DataType}' are not compatible.",
                    $"connections/{connection.Id}");
            }
        }
    }

    private static IEnumerable<ValidationIssue> ValidateContract(
        WorkflowNode node,
        NodeTypeDefinition definition)
    {
        foreach (PortDefinition registeredPort in definition.Ports)
        {
            PortDefinition? snapshotPort = node.FindPort(registeredPort.Key, registeredPort.Direction);
            if (snapshotPort is null || !ContractsMatch(snapshotPort, registeredPort))
            {
                yield return ContractMismatch(node, registeredPort.Key);
            }
        }

        foreach (PortDefinition snapshotPort in node.Ports)
        {
            PortDefinition? registeredPort = definition.FindPort(snapshotPort.Key, snapshotPort.Direction);
            if (registeredPort is null)
            {
                yield return ContractMismatch(node, snapshotPort.Key);
            }
        }
    }

    private static bool ContractsMatch(PortDefinition snapshot, PortDefinition registered) =>
        snapshot.DataType == registered.DataType &&
        snapshot.Required == registered.Required &&
        snapshot.MultiConnectionAllowed == registered.MultiConnectionAllowed;

    private static ValidationIssue ContractMismatch(WorkflowNode node, PortKey portKey) =>
        WorkflowValidationIssueFactory.Error(
            "WFL-018",
            $"Port '{portKey}' on node '{node.Id}' does not match its registered node type contract.",
            $"nodes/{node.Id}/ports/{portKey}");
}
