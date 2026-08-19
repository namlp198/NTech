using System.Collections.ObjectModel;
using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class NodeExecutionResult
{
    private readonly ReadOnlyDictionary<PortKey, DataValue> outputs;

    private NodeExecutionResult(
        NodeExecutionStatus status,
        IReadOnlyDictionary<PortKey, DataValue>? outputs,
        DomainError? error,
        WorkflowErrorCategory? errorCategory,
        NodeSkipReason? skipReason)
    {
        Status = status;
        this.outputs = new ReadOnlyDictionary<PortKey, DataValue>(
            outputs is null
                ? new Dictionary<PortKey, DataValue>()
                : new Dictionary<PortKey, DataValue>(outputs));
        Error = error;
        ErrorCategory = errorCategory;
        SkipReason = skipReason;
    }

    public NodeExecutionStatus Status { get; }

    public bool IsSuccess => Status == NodeExecutionStatus.Succeeded;

    public IReadOnlyDictionary<PortKey, DataValue> Outputs => outputs;

    public DomainError? Error { get; }

    public WorkflowErrorCategory? ErrorCategory { get; }

    public NodeSkipReason? SkipReason { get; }

    public static NodeExecutionResult Success(IReadOnlyDictionary<PortKey, DataValue>? outputs = null) =>
        new(NodeExecutionStatus.Succeeded, outputs, null, null, null);

    public static NodeExecutionResult Failure(
        DomainError error,
        WorkflowErrorCategory category = WorkflowErrorCategory.Internal) =>
        new(NodeExecutionStatus.Failed, null, error ?? throw new ArgumentNullException(nameof(error)), category, null);

    public static NodeExecutionResult TimedOut(DomainError error) =>
        new(
            NodeExecutionStatus.TimedOut,
            null,
            error ?? throw new ArgumentNullException(nameof(error)),
            WorkflowErrorCategory.Timeout,
            null);

    public static NodeExecutionResult Cancelled(DomainError error) =>
        new(
            NodeExecutionStatus.Cancelled,
            null,
            error ?? throw new ArgumentNullException(nameof(error)),
            WorkflowErrorCategory.Cancellation,
            null);

    public static NodeExecutionResult Skipped(NodeSkipReason reason) =>
        new(NodeExecutionStatus.Skipped, null, null, null, reason);
}
