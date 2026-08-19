using System.Collections.ObjectModel;
using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Validation;

public sealed class WorkflowValidationResult
{
    private readonly ReadOnlyCollection<WorkflowValidationDiagnostic> diagnostics;

    public WorkflowValidationResult(IEnumerable<WorkflowValidationDiagnostic> diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics);
        this.diagnostics = Array.AsReadOnly(diagnostics.ToArray());
    }

    public bool IsValid => diagnostics.All(item => item.Issue.Severity != ValidationSeverity.Error);

    public IReadOnlyList<WorkflowValidationDiagnostic> Diagnostics => diagnostics;

    public IReadOnlyList<ValidationIssue> Errors => diagnostics
        .Where(item => item.Issue.Severity == ValidationSeverity.Error)
        .Select(item => item.Issue)
        .ToArray();

    public IReadOnlyList<ValidationIssue> Warnings => diagnostics
        .Where(item => item.Issue.Severity == ValidationSeverity.Warning)
        .Select(item => item.Issue)
        .ToArray();
}
