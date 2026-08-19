using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class GraphValidationStage : IWorkflowValidationStage
{
    public WorkflowValidationStage Stage => WorkflowValidationStage.Graph;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        if (context.Workflow.Nodes.All(node => !node.Enabled))
        {
            yield return WorkflowValidationIssueFactory.Error(
                "WFL-021",
                "Workflow must contain at least one enabled node.",
                "nodes");
        }

        foreach (WorkflowConnection connection in context.Workflow.Connections)
        {
            WorkflowNode source = context.Workflow.Nodes.Single(node => node.Id == connection.SourceNodeId);
            WorkflowNode target = context.Workflow.Nodes.Single(node => node.Id == connection.TargetNodeId);
            if (!source.Enabled && target.Enabled)
            {
                yield return WorkflowValidationIssueFactory.Error(
                    "WFL-022",
                    $"Enabled node '{target.Id}' depends on disabled node '{source.Id}'.",
                    $"connections/{connection.Id}");
            }
        }
    }
}
