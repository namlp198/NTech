namespace NTech.NVision.Engine.Workflow.Execution;

public enum NodeSkipReason
{
    DependencyFailed,
    WorkflowAborted,
    WorkflowTimedOut,
    WorkflowCancelled,
}
