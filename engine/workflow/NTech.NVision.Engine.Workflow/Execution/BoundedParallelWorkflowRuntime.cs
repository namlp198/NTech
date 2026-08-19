using NTech.NVision.Engine.Workflow.Compilation;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class BoundedParallelWorkflowRuntime : IWorkflowRuntime, IDisposable
{
    private readonly WorkflowRuntimeScheduler scheduler;

    public BoundedParallelWorkflowRuntime(
        int maxConcurrency,
        TimeProvider? timeProvider = null,
        IExecutionTraceSink? traceSink = null)
    {
        if (maxConcurrency <= 0)
        {
            throw new ArgumentOutOfRangeException(nameof(maxConcurrency), "Maximum concurrency must be positive.");
        }

        MaxConcurrency = maxConcurrency;
        scheduler = new WorkflowRuntimeScheduler(maxConcurrency, timeProvider, traceSink);
    }

    public int MaxConcurrency { get; }

    public ValueTask<WorkflowExecutionResult> ExecuteAsync(
        ExecutionPlan plan,
        CancellationToken cancellationToken = default) =>
        scheduler.ExecuteAsync(plan, cancellationToken);

    public void Dispose() => scheduler.Dispose();
}
