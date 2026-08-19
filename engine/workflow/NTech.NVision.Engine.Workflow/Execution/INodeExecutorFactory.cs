namespace NTech.NVision.Engine.Workflow.Execution;

public interface INodeExecutorFactory
{
    public INodeExecutor CreateExecutor();
}
