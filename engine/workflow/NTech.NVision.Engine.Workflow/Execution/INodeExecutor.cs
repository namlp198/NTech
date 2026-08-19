namespace NTech.NVision.Engine.Workflow.Execution;

public interface INodeExecutor
{
    public ValueTask<NodeExecutionResult> ExecuteAsync(
        NodeExecutionContext context,
        CancellationToken cancellationToken);
}
