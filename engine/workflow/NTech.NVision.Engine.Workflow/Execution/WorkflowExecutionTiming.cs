namespace NTech.NVision.Engine.Workflow.Execution;

public sealed record WorkflowExecutionTiming(
    DateTimeOffset StartedAt,
    DateTimeOffset CompletedAt,
    TimeSpan Duration);
