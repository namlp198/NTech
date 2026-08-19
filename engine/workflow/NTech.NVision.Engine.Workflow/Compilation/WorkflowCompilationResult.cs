using System.Collections.ObjectModel;
using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed class WorkflowCompilationResult
{
    private readonly ReadOnlyCollection<ValidationIssue> issues;

    public WorkflowCompilationResult(ExecutionPlan? plan, IEnumerable<ValidationIssue> issues)
    {
        Plan = plan;
        this.issues = Array.AsReadOnly(issues?.ToArray() ?? throw new ArgumentNullException(nameof(issues)));
    }

    public bool IsSuccess => Plan is not null && issues.All(issue => issue.Severity != ValidationSeverity.Error);

    public ExecutionPlan? Plan { get; }

    public IReadOnlyList<ValidationIssue> Issues => issues;
}
