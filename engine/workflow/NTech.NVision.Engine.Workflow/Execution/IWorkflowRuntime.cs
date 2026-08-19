using NTech.NVision.Engine.Workflow.Compilation;

namespace NTech.NVision.Engine.Workflow.Execution;

public interface IWorkflowRuntime
{
    public ValueTask<WorkflowExecutionResult> ExecuteAsync(
        ExecutionPlan plan,
        CancellationToken cancellationToken = default);
}
