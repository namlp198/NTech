using System.Collections.ObjectModel;
using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Registry;
using NTech.NVision.Engine.Workflow.Validation.Stages;

namespace NTech.NVision.Engine.Workflow.Validation;

public sealed class WorkflowValidator : IWorkflowValidator
{
    private readonly ReadOnlyCollection<IWorkflowValidationStage> stages;
    private readonly INodeTypeRegistry registry;

    public WorkflowValidator(
        INodeTypeRegistry registry,
        IEnumerable<IWorkflowValidationStage>? stages = null)
    {
        this.registry = registry ?? throw new ArgumentNullException(nameof(registry));
        IWorkflowValidationStage[] stageSnapshot = (stages ?? CreateDefaultStages())
            .OrderBy(stage => stage.Stage)
            .ToArray();
        if (stageSnapshot.GroupBy(stage => stage.Stage).Any(group => group.Count() > 1))
        {
            throw new ArgumentException("Only one validator may be registered for each Workflow validation stage.", nameof(stages));
        }

        this.stages = Array.AsReadOnly(stageSnapshot);
    }

    public IReadOnlyList<IWorkflowValidationStage> Stages => stages;

    public WorkflowValidationResult Validate(WorkflowDefinition workflow)
    {
        ArgumentNullException.ThrowIfNull(workflow);
        var context = new WorkflowValidationContext(workflow, registry);
        var diagnostics = new List<WorkflowValidationDiagnostic>();
        foreach (IWorkflowValidationStage stage in stages)
        {
            IEnumerable<ValidationIssue> issues = stage.Validate(context)
                ?? throw new InvalidOperationException($"Validation stage '{stage.Stage}' returned null.");
            diagnostics.AddRange(issues.Select(issue => new WorkflowValidationDiagnostic(stage.Stage, issue)));
        }

        return new WorkflowValidationResult(diagnostics);
    }

    private static IEnumerable<IWorkflowValidationStage> CreateDefaultStages() =>
    [
        new SchemaValidationStage(),
        new GraphValidationStage(),
        new NodeTypeValidationStage(),
        new PortValidationStage(),
        new ParameterValidationStage(),
        new ReferenceValidationStage(),
        new ExecutionConstraintValidationStage(),
    ];
}
