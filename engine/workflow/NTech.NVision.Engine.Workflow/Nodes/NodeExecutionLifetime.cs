namespace NTech.NVision.Engine.Workflow.Nodes;

public enum NodeExecutionLifetime
{
    Stateless,
    PerExecution,
    PerSession,
    SingletonShared,
}
