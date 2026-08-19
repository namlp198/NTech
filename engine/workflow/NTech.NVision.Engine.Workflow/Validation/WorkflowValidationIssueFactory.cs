using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Validation;

internal static class WorkflowValidationIssueFactory
{
    public static ValidationIssue Error(string code, string message, string? path = null) =>
        new(new DomainError(code, message), ValidationSeverity.Error, path);

    public static ValidationIssue Warning(string code, string message, string? path = null) =>
        new(new DomainError(code, message), ValidationSeverity.Warning, path);
}
