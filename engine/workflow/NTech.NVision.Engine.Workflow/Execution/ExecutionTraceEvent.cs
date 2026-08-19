using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed record ExecutionTraceEvent(
    ExecutionTraceEventKind Kind,
    DateTimeOffset Timestamp,
    WorkflowExecutionId ExecutionId,
    WorkflowId WorkflowId,
    NodeId? NodeId = null,
    NodeTypeId? NodeTypeId = null,
    NodeExecutionStatus? NodeStatus = null,
    WorkflowExecutionStatus? WorkflowStatus = null,
    DomainError? Error = null,
    TimeSpan? Duration = null,
    NodeSkipReason? SkipReason = null);
