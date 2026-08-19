using System.Collections.ObjectModel;
using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Compilation;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class WorkflowExecutionResult
{
    private readonly ReadOnlyCollection<NodeExecutionRecord> nodeResults;
    private readonly ReadOnlyDictionary<PortAddress, DataValue> outputs;
    private readonly ReadOnlyCollection<WorkflowExecutionError> errors;
    private readonly ReadOnlyCollection<DomainError> warnings;

    public WorkflowExecutionResult(
        WorkflowExecutionId executionId,
        WorkflowId workflowId,
        WorkflowExecutionStatus status,
        IEnumerable<NodeExecutionRecord> nodeResults,
        IReadOnlyDictionary<PortAddress, DataValue> outputs,
        DomainError? error = null,
        IEnumerable<WorkflowExecutionError>? errors = null,
        IEnumerable<DomainError>? warnings = null,
        WorkflowExecutionTiming? timing = null)
    {
        if (status == WorkflowExecutionStatus.Succeeded && error is not null)
        {
            throw new ArgumentException("A succeeded Workflow execution cannot contain an error.", nameof(error));
        }

        if (status != WorkflowExecutionStatus.Succeeded && error is null)
        {
            throw new ArgumentException("A non-succeeded Workflow execution must contain an error.", nameof(error));
        }

        ExecutionId = executionId ?? throw new ArgumentNullException(nameof(executionId));
        WorkflowId = workflowId ?? throw new ArgumentNullException(nameof(workflowId));
        Status = status;
        this.nodeResults = Array.AsReadOnly(nodeResults?.ToArray() ?? throw new ArgumentNullException(nameof(nodeResults)));
        this.outputs = new ReadOnlyDictionary<PortAddress, DataValue>(
            outputs is null
                ? throw new ArgumentNullException(nameof(outputs))
                : new Dictionary<PortAddress, DataValue>(outputs));
        Error = error;
        this.errors = Array.AsReadOnly(errors?.ToArray() ?? []);
        this.warnings = Array.AsReadOnly(warnings?.ToArray() ?? []);
        Timing = timing;
    }

    public WorkflowExecutionId ExecutionId { get; }

    public WorkflowId WorkflowId { get; }

    public WorkflowExecutionStatus Status { get; }

    public bool IsSuccess => Status == WorkflowExecutionStatus.Succeeded;

    public IReadOnlyList<NodeExecutionRecord> NodeResults => nodeResults;

    public IReadOnlyDictionary<PortAddress, DataValue> Outputs => outputs;

    public DomainError? Error { get; }

    public IReadOnlyList<WorkflowExecutionError> Errors => errors;

    public IReadOnlyList<DomainError> Warnings => warnings;

    public WorkflowExecutionTiming? Timing { get; }
}
