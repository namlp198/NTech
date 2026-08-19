namespace NTech.NVision.Engine.Workflow.Execution;

public enum ExecutionTraceEventKind
{
    WorkflowStarted,
    NodeStarted,
    NodeCompleted,
    NodeFailed,
    NodeTimedOut,
    NodeCancelled,
    NodeSkipped,
    WorkflowCompleted,
}
