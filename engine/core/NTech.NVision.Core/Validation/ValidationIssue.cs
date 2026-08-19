using NTech.NVision.Core.Errors;

namespace NTech.NVision.Core.Validation;

/// <summary>Describes a validation problem without throwing from normal validation flow.</summary>
public sealed record ValidationIssue
{
    public ValidationIssue(DomainError error, ValidationSeverity severity, string? path = null)
    {
        Error = error ?? throw new ArgumentNullException(nameof(error));
        Severity = severity;
        Path = string.IsNullOrWhiteSpace(path) ? null : path.Trim();
    }

    public DomainError Error { get; }

    public ValidationSeverity Severity { get; }

    public string? Path { get; }
}
