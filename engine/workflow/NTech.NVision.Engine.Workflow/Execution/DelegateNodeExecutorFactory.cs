namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class DelegateNodeExecutorFactory : INodeExecutorFactory
{
    private readonly Func<INodeExecutor> factory;

    public DelegateNodeExecutorFactory(Func<INodeExecutor> factory) =>
        this.factory = factory ?? throw new ArgumentNullException(nameof(factory));

    public INodeExecutor CreateExecutor() =>
        factory() ?? throw new InvalidOperationException("Node executor factory delegate returned null.");
}
