using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class ExecutionConstraintValidationStage : IWorkflowValidationStage
{
    public WorkflowValidationStage Stage => WorkflowValidationStage.ExecutionConstraint;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        foreach (WorkflowNode node in context.Workflow.Nodes.Where(node => node.Enabled))
        {
            if (context.Workflow.Timeout is not null && node.Timeout > context.Workflow.Timeout)
            {
                yield return WorkflowValidationIssueFactory.Error(
                    "WFL-031",
                    $"Node '{node.Id}' timeout exceeds the Workflow timeout.",
                    $"nodes/{node.Id}/timeout");
            }

            var resolution = context.Registry.Resolve(node.NodeTypeId, node.NodeTypeVersion);
            if (resolution.IsFailure)
            {
                continue;
            }

            var characteristics = resolution.Value.ExecutionCharacteristics;
            if (characteristics.MayBlock && !characteristics.SupportsCancellation)
            {
                yield return WorkflowValidationIssueFactory.Warning(
                    "WFL-030",
                    $"Blocking node '{node.Id}' does not declare cancellation support.",
                    $"nodes/{node.Id}/executionCharacteristics");
            }
        }
    }
}
