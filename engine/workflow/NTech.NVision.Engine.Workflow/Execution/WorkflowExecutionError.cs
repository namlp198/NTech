using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed record WorkflowExecutionError
{
    public WorkflowExecutionError(
        WorkflowExecutionId executionId,
        WorkflowId workflowId,
        DomainError error,
        WorkflowErrorCategory category,
        DateTimeOffset occurredAt,
        NodeId? nodeId = null,
        NodeTypeId? nodeTypeId = null,
        bool recoverable = false)
    {
        ExecutionId = executionId ?? throw new ArgumentNullException(nameof(executionId));
        WorkflowId = workflowId ?? throw new ArgumentNullException(nameof(workflowId));
        Error = error ?? throw new ArgumentNullException(nameof(error));
        Category = category;
        OccurredAt = occurredAt;
        NodeId = nodeId;
        NodeTypeId = nodeTypeId;
        Recoverable = recoverable;
    }

    public WorkflowExecutionId ExecutionId { get; }

    public WorkflowId WorkflowId { get; }

    public NodeId? NodeId { get; }

    public NodeTypeId? NodeTypeId { get; }

    public DomainError Error { get; }

    public WorkflowErrorCategory Category { get; }

    public DateTimeOffset OccurredAt { get; }

    public bool Recoverable { get; }
}
