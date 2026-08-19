using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class NodeTypeValidationStage : IWorkflowValidationStage
{
    public WorkflowValidationStage Stage => WorkflowValidationStage.NodeType;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        foreach (WorkflowNode node in context.Workflow.Nodes)
        {
            var resolution = context.Registry.Resolve(node.NodeTypeId, node.NodeTypeVersion);
            if (resolution.IsFailure)
            {
                yield return new ValidationIssue(
                    resolution.Error!,
                    ValidationSeverity.Error,
                    $"nodes/{node.Id}/nodeType");
            }
        }
    }
}
