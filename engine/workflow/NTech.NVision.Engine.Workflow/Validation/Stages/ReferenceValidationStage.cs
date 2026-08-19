using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class ReferenceValidationStage : IWorkflowValidationStage
{
    public WorkflowValidationStage Stage => WorkflowValidationStage.Reference;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        return [];
    }
}
