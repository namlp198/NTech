using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Validation;

public interface IWorkflowValidationStage
{
    public WorkflowValidationStage Stage { get; }

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context);
}
