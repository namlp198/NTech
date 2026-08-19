namespace NTech.NVision.Engine.Workflow.Execution;

public sealed record NodeExecutionTiming(
    DateTimeOffset StartedAt,
    DateTimeOffset CompletedAt,
    TimeSpan Duration);
