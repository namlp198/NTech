namespace NTech.NVision.Engine.Workflow.Execution;

public interface IExecutionTraceSink
{
    public ValueTask WriteAsync(ExecutionTraceEvent traceEvent, CancellationToken cancellationToken = default);
}
