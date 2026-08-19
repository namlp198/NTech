using NTech.NVision.Engine.Workflow.Compilation;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class SequentialWorkflowRuntime : IWorkflowRuntime, IDisposable
{
    private readonly WorkflowRuntimeScheduler scheduler;

    public SequentialWorkflowRuntime(
        TimeProvider? timeProvider = null,
        IExecutionTraceSink? traceSink = null) =>
        scheduler = new WorkflowRuntimeScheduler(1, timeProvider, traceSink);

    public ValueTask<WorkflowExecutionResult> ExecuteAsync(
        ExecutionPlan plan,
        CancellationToken cancellationToken = default) =>
        scheduler.ExecuteAsync(plan, cancellationToken);

    public void Dispose() => scheduler.Dispose();
}
