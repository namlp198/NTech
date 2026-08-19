using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class SchemaValidationStage : IWorkflowValidationStage
{
    private readonly HashSet<string> supportedVersions;

    public SchemaValidationStage(IEnumerable<string>? supportedVersions = null)
    {
        this.supportedVersions = new HashSet<string>(supportedVersions ?? ["1.0"], StringComparer.Ordinal);
        if (this.supportedVersions.Count == 0)
        {
            throw new ArgumentException("At least one supported schema version is required.", nameof(supportedVersions));
        }
    }

    public WorkflowValidationStage Stage => WorkflowValidationStage.Schema;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        if (!supportedVersions.Contains(context.Workflow.SchemaVersion))
        {
            yield return WorkflowValidationIssueFactory.Error(
                "WFL-020",
                $"Workflow schema version '{context.Workflow.SchemaVersion}' is not supported.",
                "schemaVersion");
        }
    }
}
