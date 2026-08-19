using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Validation;

public interface IWorkflowValidator
{
    public WorkflowValidationResult Validate(WorkflowDefinition workflow);
}
