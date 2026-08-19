namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class NullExecutionTraceSink : IExecutionTraceSink
{
    public static NullExecutionTraceSink Instance { get; } = new();

    private NullExecutionTraceSink()
    {
    }

    public ValueTask WriteAsync(ExecutionTraceEvent traceEvent, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(traceEvent);
        return ValueTask.CompletedTask;
    }
}
