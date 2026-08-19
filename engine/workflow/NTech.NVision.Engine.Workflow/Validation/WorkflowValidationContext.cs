using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Workflow.Validation;

public sealed class WorkflowValidationContext
{
    public WorkflowValidationContext(WorkflowDefinition workflow, INodeTypeRegistry registry)
    {
        Workflow = workflow ?? throw new ArgumentNullException(nameof(workflow));
        Registry = registry ?? throw new ArgumentNullException(nameof(registry));
    }

    public WorkflowDefinition Workflow { get; }

    public INodeTypeRegistry Registry { get; }
}
