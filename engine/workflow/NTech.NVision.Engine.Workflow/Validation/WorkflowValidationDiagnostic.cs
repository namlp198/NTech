using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Validation;

public sealed record WorkflowValidationDiagnostic(
    WorkflowValidationStage Stage,
    ValidationIssue Issue);
