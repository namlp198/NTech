using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Compilation;

namespace NTech.NVision.Engine.Workflow.Execution;

internal sealed class WorkflowRuntimeScheduler : IWorkflowRuntime, IDisposable
{
    private readonly int maxConcurrency;
    private readonly TimeProvider timeProvider;
    private readonly SemaphoreSlim traceGate = new(1, 1);
    private readonly IExecutionTraceSink traceSink;

    public WorkflowRuntimeScheduler(
        int maxConcurrency,
        TimeProvider? timeProvider = null,
        IExecutionTraceSink? traceSink = null)
    {
        if (maxConcurrency <= 0)
        {
            throw new ArgumentOutOfRangeException(nameof(maxConcurrency), "Maximum concurrency must be positive.");
        }

        this.maxConcurrency = maxConcurrency;
        this.timeProvider = timeProvider ?? TimeProvider.System;
        this.traceSink = traceSink ?? NullExecutionTraceSink.Instance;
    }

    public async ValueTask<WorkflowExecutionResult> ExecuteAsync(
        ExecutionPlan plan,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(plan);
        WorkflowExecutionId executionId = WorkflowExecutionId.New();
        DateTimeOffset workflowStartedAt = timeProvider.GetUtcNow();
        long workflowStartedTimestamp = timeProvider.GetTimestamp();
        var records = new Dictionary<NodeId, NodeExecutionRecord>();
        var resultsByNode = new Dictionary<NodeId, NodeExecutionResult>();
        var errors = new List<WorkflowExecutionError>();
        var warnings = new List<DomainError>();
        var store = new PortValueStore();
        Dictionary<NodeId, int> planOrder = plan.Nodes
            .Select((node, index) => (node.NodeId, index))
            .ToDictionary(item => item.NodeId, item => item.index);
        Dictionary<NodeId, int> remainingDependencies = plan.Nodes
            .ToDictionary(node => node.NodeId, node => node.Dependencies.Count);
        var ready = new SortedSet<int>(
            plan.Nodes
                .Select((node, index) => (node, index))
                .Where(item => item.node.Dependencies.Count == 0)
                .Select(item => item.index));
        var running = new Dictionary<NodeId, Task<NodeRunResult>>();
        SchedulerTermination? termination = null;

        using CancellationTokenSource? workflowTimeoutSource = plan.Timeout is TimeSpan workflowTimeout
            ? new CancellationTokenSource(workflowTimeout, timeProvider)
            : null;
        using var workflowCancellationSource = workflowTimeoutSource is null
            ? CancellationTokenSource.CreateLinkedTokenSource(cancellationToken)
            : CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, workflowTimeoutSource.Token);
        CancellationToken workflowToken = workflowCancellationSource.Token;
        using var fatalCancellationSource = new CancellationTokenSource();
        using var executionCancellationSource = CancellationTokenSource.CreateLinkedTokenSource(
            workflowToken,
            fatalCancellationSource.Token);

        await WriteTraceAsync(
            new ExecutionTraceEvent(
                ExecutionTraceEventKind.WorkflowStarted,
                workflowStartedAt,
                executionId,
                plan.WorkflowId),
            warnings).ConfigureAwait(false);

        while (records.Count < plan.Nodes.Count)
        {
            if (termination is null && workflowToken.IsCancellationRequested)
            {
                bool timedOut = workflowTimeoutSource?.IsCancellationRequested == true &&
                    !cancellationToken.IsCancellationRequested;
                DomainError cancellationError = timedOut
                    ? new DomainError("EXE-009", $"Workflow '{plan.WorkflowId}' exceeded its timeout of '{plan.Timeout}'.")
                    : new DomainError("EXE-004", $"Workflow '{plan.WorkflowId}' execution was cancelled.");
                termination = new SchedulerTermination(
                    timedOut ? WorkflowExecutionStatus.TimedOut : WorkflowExecutionStatus.Cancelled,
                    timedOut ? NodeSkipReason.WorkflowTimedOut : NodeSkipReason.WorkflowCancelled,
                    cancellationError);
                errors.Add(CreateWorkflowError(
                    plan,
                    executionId,
                    cancellationError,
                    timedOut ? WorkflowErrorCategory.Timeout : WorkflowErrorCategory.Cancellation));
            }

            while (termination is null && ready.Count > 0 && running.Count < maxConcurrency)
            {
                int nodeIndex = ready.Min;
                ready.Remove(nodeIndex);
                CompiledNode node = plan.Nodes[nodeIndex];
                if (node.Dependencies.Any(dependency => !resultsByNode[dependency].IsSuccess))
                {
                    await RecordSkippedAsync(
                        plan,
                        executionId,
                        node,
                        NodeSkipReason.DependencyFailed,
                        records,
                        resultsByNode,
                        remainingDependencies,
                        ready,
                        planOrder,
                        warnings).ConfigureAwait(false);
                    continue;
                }

                running[node.NodeId] = ExecuteNodeAsync(
                    plan,
                    executionId,
                    node,
                    store,
                    workflowTimeoutSource,
                    warnings,
                    cancellationToken,
                    executionCancellationSource.Token).AsTask();
            }

            if (running.Count == 0)
            {
                if (termination is not null)
                {
                    await RecordPendingAsSkippedAsync(
                        plan,
                        executionId,
                        termination.SkipReason,
                        records,
                        resultsByNode,
                        warnings).ConfigureAwait(false);
                    break;
                }

                if (records.Count != plan.Nodes.Count)
                {
                    DomainError schedulerError = new(
                        "EXE-011",
                        "Workflow scheduler reached an invalid state with no ready or running nodes.");
                    termination = new SchedulerTermination(
                        WorkflowExecutionStatus.Failed,
                        NodeSkipReason.WorkflowAborted,
                        schedulerError);
                    errors.Add(CreateWorkflowError(
                        plan,
                        executionId,
                        schedulerError,
                        WorkflowErrorCategory.Internal));
                    continue;
                }

                break;
            }

            await Task.WhenAny(running.Values).ConfigureAwait(false);
            NodeId[] completedNodeIds = running
                .Where(pair => pair.Value.IsCompleted)
                .Select(pair => pair.Key)
                .OrderBy(nodeId => planOrder[nodeId])
                .ToArray();

            foreach (NodeId nodeId in completedNodeIds)
            {
                Task<NodeRunResult> task = running[nodeId];
                running.Remove(nodeId);
                NodeRunResult run = await task.ConfigureAwait(false);
                CompiledNode node = plan.Nodes[planOrder[nodeId]];
                records[nodeId] = new NodeExecutionRecord(nodeId, run.Result, run.Timing);
                resultsByNode[nodeId] = run.Result;
                ReleaseDependents(node, remainingDependencies, ready, planOrder);

                if (run.Result.IsSuccess || termination is not null)
                {
                    continue;
                }

                DomainError error = run.Result.Error ??
                    new DomainError("EXE-006", $"Node '{node.NodeId}' failed without an error.");
                bool externallyCancelled = cancellationToken.IsCancellationRequested;
                bool workflowTimedOut = workflowTimeoutSource?.IsCancellationRequested == true &&
                    !externallyCancelled;
                bool recoverable = !externallyCancelled && !workflowTimedOut &&
                    node.FailurePolicy == NodeFailurePolicy.Continue;
                errors.Add(CreateWorkflowError(
                    plan,
                    executionId,
                    error,
                    run.Result.ErrorCategory ?? WorkflowErrorCategory.Internal,
                    node,
                    recoverable));

                if (recoverable)
                {
                    continue;
                }

                if (externallyCancelled)
                {
                    termination = new SchedulerTermination(
                        WorkflowExecutionStatus.Cancelled,
                        NodeSkipReason.WorkflowCancelled,
                        error);
                }
                else if (workflowTimedOut || run.Result.Status == NodeExecutionStatus.TimedOut)
                {
                    termination = new SchedulerTermination(
                        WorkflowExecutionStatus.TimedOut,
                        NodeSkipReason.WorkflowTimedOut,
                        error);
                }
                else
                {
                    termination = new SchedulerTermination(
                        WorkflowExecutionStatus.Failed,
                        NodeSkipReason.WorkflowAborted,
                        error);
                }

                CancelAfterFatalFailure(fatalCancellationSource, warnings);
            }
        }

        NodeExecutionRecord[] orderedResults = plan.Nodes
            .Select(node => records[node.NodeId])
            .ToArray();
        return await CompleteAsync(
            plan,
            executionId,
            termination?.Status ?? WorkflowExecutionStatus.Succeeded,
            orderedResults,
            store,
            termination?.Error,
            errors,
            warnings,
            workflowStartedAt,
            workflowStartedTimestamp).ConfigureAwait(false);
    }

    public void Dispose() => traceGate.Dispose();

    private async ValueTask RecordSkippedAsync(
        ExecutionPlan plan,
        WorkflowExecutionId executionId,
        CompiledNode node,
        NodeSkipReason reason,
        Dictionary<NodeId, NodeExecutionRecord> records,
        Dictionary<NodeId, NodeExecutionResult> resultsByNode,
        Dictionary<NodeId, int> remainingDependencies,
        SortedSet<int> ready,
        Dictionary<NodeId, int> planOrder,
        List<DomainError> warnings)
    {
        NodeExecutionResult skipped = NodeExecutionResult.Skipped(reason);
        records[node.NodeId] = new NodeExecutionRecord(node.NodeId, skipped);
        resultsByNode[node.NodeId] = skipped;
        ReleaseDependents(node, remainingDependencies, ready, planOrder);
        await WriteNodeTraceAsync(plan, executionId, node, skipped, null, warnings).ConfigureAwait(false);
    }

    private async ValueTask RecordPendingAsSkippedAsync(
        ExecutionPlan plan,
        WorkflowExecutionId executionId,
        NodeSkipReason reason,
        Dictionary<NodeId, NodeExecutionRecord> records,
        Dictionary<NodeId, NodeExecutionResult> resultsByNode,
        List<DomainError> warnings)
    {
        foreach (CompiledNode node in plan.Nodes.Where(node => !records.ContainsKey(node.NodeId)))
        {
            NodeExecutionResult skipped = NodeExecutionResult.Skipped(reason);
            records[node.NodeId] = new NodeExecutionRecord(node.NodeId, skipped);
            resultsByNode[node.NodeId] = skipped;
            await WriteNodeTraceAsync(plan, executionId, node, skipped, null, warnings).ConfigureAwait(false);
        }
    }

    private static void ReleaseDependents(
        CompiledNode completed,
        Dictionary<NodeId, int> remainingDependencies,
        SortedSet<int> ready,
        Dictionary<NodeId, int> planOrder)
    {
        foreach (NodeId dependent in completed.Dependents)
        {
            remainingDependencies[dependent]--;
            if (remainingDependencies[dependent] == 0)
            {
                ready.Add(planOrder[dependent]);
            }
        }
    }

    private static void CancelAfterFatalFailure(
        CancellationTokenSource cancellationSource,
        List<DomainError> warnings)
    {
        try
        {
            cancellationSource.Cancel();
        }
        catch (AggregateException exception)
        {
            warnings.Add(new DomainError(
                "EXE-012",
                $"One or more node cancellation callbacks failed: {exception.GetBaseException().Message}"));
        }
    }

    private async ValueTask<NodeRunResult> ExecuteNodeAsync(
        ExecutionPlan plan,
        WorkflowExecutionId executionId,
        CompiledNode node,
        PortValueStore store,
        CancellationTokenSource? workflowTimeoutSource,
        List<DomainError> warnings,
        CancellationToken externalToken,
        CancellationToken workflowToken)
    {
        DateTimeOffset startedAt = timeProvider.GetUtcNow();
        long startedTimestamp = timeProvider.GetTimestamp();
        await WriteTraceAsync(
            new ExecutionTraceEvent(
                ExecutionTraceEventKind.NodeStarted,
                startedAt,
                executionId,
                plan.WorkflowId,
                node.NodeId,
                node.NodeType.Id),
            warnings).ConfigureAwait(false);

        NodeExecutionResult result;
        InputBuildResult inputResult = BuildInputs(node, store);
        if (inputResult.Error is not null)
        {
            result = NodeExecutionResult.Failure(inputResult.Error, WorkflowErrorCategory.Input);
        }
        else
        {
            using CancellationTokenSource? nodeTimeoutSource = node.Timeout is TimeSpan nodeTimeout
                ? new CancellationTokenSource(nodeTimeout, timeProvider)
                : null;
            using var nodeCancellationSource = nodeTimeoutSource is null
                ? CancellationTokenSource.CreateLinkedTokenSource(workflowToken)
                : CancellationTokenSource.CreateLinkedTokenSource(workflowToken, nodeTimeoutSource.Token);
            Task<NodeExecutionResult>? executorTask = null;
            try
            {
                INodeExecutor executor = node.ExecutorFactory.CreateExecutor()
                    ?? throw new InvalidOperationException($"Executor factory for node '{node.NodeId}' returned null.");
                var context = new NodeExecutionContext(node.NodeId, inputResult.Inputs!, node.BoundParameters);
                executorTask = Task.Run(
                    async () => await executor
                        .ExecuteAsync(context, nodeCancellationSource.Token)
                        .ConfigureAwait(false),
                    CancellationToken.None);
                result = await executorTask.WaitAsync(nodeCancellationSource.Token).ConfigureAwait(false)
                    ?? throw new InvalidOperationException($"Executor for node '{node.NodeId}' returned null.");
            }
            catch (OperationCanceledException) when (externalToken.IsCancellationRequested)
            {
                ObserveAbandonedTask(executorTask);
                result = NodeExecutionResult.Cancelled(
                    new DomainError("EXE-004", $"Node '{node.NodeId}' execution was cancelled."));
            }
            catch (OperationCanceledException) when (workflowTimeoutSource?.IsCancellationRequested == true)
            {
                ObserveAbandonedTask(executorTask);
                result = NodeExecutionResult.TimedOut(
                    new DomainError("EXE-009", $"Workflow timeout elapsed while node '{node.NodeId}' was running."));
            }
            catch (OperationCanceledException) when (nodeTimeoutSource?.IsCancellationRequested == true)
            {
                ObserveAbandonedTask(executorTask);
                result = NodeExecutionResult.TimedOut(
                    new DomainError("EXE-008", $"Node '{node.NodeId}' exceeded its timeout of '{node.Timeout}'."));
            }
            catch (OperationCanceledException)
            {
                result = NodeExecutionResult.Cancelled(
                    new DomainError("EXE-010", $"Node '{node.NodeId}' cancelled its execution."));
            }
            catch (Exception exception) when (exception is not OutOfMemoryException)
            {
                result = NodeExecutionResult.Failure(
                    new DomainError("EXE-005", $"Node '{node.NodeId}' threw an exception: {exception.Message}"));
            }

            if (result.IsSuccess)
            {
                DomainError? outputError = StoreOutputs(node, result, store);
                if (outputError is not null)
                {
                    result = NodeExecutionResult.Failure(outputError, WorkflowErrorCategory.Internal);
                }
            }
        }

        DateTimeOffset completedAt = timeProvider.GetUtcNow();
        TimeSpan duration = timeProvider.GetElapsedTime(startedTimestamp, timeProvider.GetTimestamp());
        var timing = new NodeExecutionTiming(startedAt, completedAt, duration);
        await WriteNodeTraceAsync(plan, executionId, node, result, timing, warnings).ConfigureAwait(false);
        return new NodeRunResult(result, timing);
    }

    private async ValueTask<WorkflowExecutionResult> CompleteAsync(
        ExecutionPlan plan,
        WorkflowExecutionId executionId,
        WorkflowExecutionStatus status,
        IEnumerable<NodeExecutionRecord> results,
        PortValueStore store,
        DomainError? error,
        IEnumerable<WorkflowExecutionError> errors,
        List<DomainError> warnings,
        DateTimeOffset startedAt,
        long startedTimestamp)
    {
        DateTimeOffset completedAt = timeProvider.GetUtcNow();
        TimeSpan duration = timeProvider.GetElapsedTime(startedTimestamp, timeProvider.GetTimestamp());
        var timing = new WorkflowExecutionTiming(startedAt, completedAt, duration);
        await WriteTraceAsync(
            new ExecutionTraceEvent(
                ExecutionTraceEventKind.WorkflowCompleted,
                completedAt,
                executionId,
                plan.WorkflowId,
                WorkflowStatus: status,
                Error: error,
                Duration: duration),
            warnings).ConfigureAwait(false);
        return new WorkflowExecutionResult(
            executionId,
            plan.WorkflowId,
            status,
            results,
            store.Snapshot(plan.TerminalNodes),
            error,
            errors,
            warnings,
            timing);
    }

    private async ValueTask WriteNodeTraceAsync(
        ExecutionPlan plan,
        WorkflowExecutionId executionId,
        CompiledNode node,
        NodeExecutionResult result,
        NodeExecutionTiming? timing,
        List<DomainError> warnings)
    {
        ExecutionTraceEventKind kind = result.Status switch
        {
            NodeExecutionStatus.Succeeded => ExecutionTraceEventKind.NodeCompleted,
            NodeExecutionStatus.Failed => ExecutionTraceEventKind.NodeFailed,
            NodeExecutionStatus.TimedOut => ExecutionTraceEventKind.NodeTimedOut,
            NodeExecutionStatus.Cancelled => ExecutionTraceEventKind.NodeCancelled,
            NodeExecutionStatus.Skipped => ExecutionTraceEventKind.NodeSkipped,
            _ => throw new ArgumentOutOfRangeException(nameof(result)),
        };
        await WriteTraceAsync(
            new ExecutionTraceEvent(
                kind,
                timing?.CompletedAt ?? timeProvider.GetUtcNow(),
                executionId,
                plan.WorkflowId,
                node.NodeId,
                node.NodeType.Id,
                result.Status,
                Error: result.Error,
                Duration: timing?.Duration,
                SkipReason: result.SkipReason),
            warnings).ConfigureAwait(false);
    }

    private async ValueTask WriteTraceAsync(
        ExecutionTraceEvent traceEvent,
        List<DomainError> warnings)
    {
        await traceGate.WaitAsync(CancellationToken.None).ConfigureAwait(false);
        try
        {
            try
            {
                await traceSink.WriteAsync(traceEvent, CancellationToken.None).ConfigureAwait(false);
            }
            catch (Exception exception) when (exception is not OutOfMemoryException)
            {
                if (!warnings.Any(warning => warning.Code == "TRC-001"))
                {
                    warnings.Add(new DomainError(
                        "TRC-001",
                        $"Execution trace sink failed; execution continued without complete trace data: {exception.Message}"));
                }
            }
        }
        finally
        {
            traceGate.Release();
        }
    }

    private WorkflowExecutionError CreateWorkflowError(
        ExecutionPlan plan,
        WorkflowExecutionId executionId,
        DomainError error,
        WorkflowErrorCategory category,
        CompiledNode? node = null,
        bool recoverable = false) =>
        new(
            executionId,
            plan.WorkflowId,
            error,
            category,
            timeProvider.GetUtcNow(),
            node?.NodeId,
            node?.NodeType.Id,
            recoverable);

    private static InputBuildResult BuildInputs(CompiledNode node, PortValueStore store)
    {
        var inputs = new Dictionary<PortKey, IReadOnlyList<DataValue>>();
        foreach (IGrouping<PortKey, CompiledInputBinding> bindings in node.InputBindings.GroupBy(binding => binding.TargetPort))
        {
            var values = new List<DataValue>();
            foreach (CompiledInputBinding binding in bindings)
            {
                if (!store.TryGet(binding.Source, out DataValue? value))
                {
                    return new InputBuildResult(
                        null,
                        new DomainError(
                            "EXE-001",
                            $"Input '{binding.TargetPort}' for node '{node.NodeId}' has no value from '{binding.Source.NodeId}/{binding.Source.Port}'."));
                }

                values.Add(value);
            }

            inputs[bindings.Key] = values.AsReadOnly();
        }

        return new InputBuildResult(inputs, null);
    }

    private static DomainError? StoreOutputs(CompiledNode node, NodeExecutionResult result, PortValueStore store)
    {
        foreach ((PortKey portKey, DataValue value) in result.Outputs)
        {
            PortDefinition? definition = node.NodeType.FindPort(portKey, PortDirection.Output);
            if (definition is null)
            {
                return new DomainError(
                    "EXE-002",
                    $"Node '{node.NodeId}' produced undeclared output port '{portKey}'.");
            }

            if (definition.DataType != value.DataType)
            {
                return new DomainError(
                    "EXE-003",
                    $"Node '{node.NodeId}' output '{portKey}' produced '{value.DataType}' instead of '{definition.DataType}'.");
            }

            store.Set(new PortAddress(node.NodeId, portKey), value);
        }

        foreach (PortDefinition output in node.NodeType.OutputPorts.Where(port => port.Required))
        {
            if (!result.Outputs.ContainsKey(output.Key))
            {
                return new DomainError(
                    "EXE-007",
                    $"Node '{node.NodeId}' did not produce required output '{output.Key}'.");
            }
        }

        return null;
    }

    private static void ObserveAbandonedTask(Task<NodeExecutionResult>? task)
    {
        if (task is null || task.IsCompleted)
        {
            return;
        }

        _ = task.ContinueWith(
            completed => _ = completed.Exception,
            CancellationToken.None,
            TaskContinuationOptions.OnlyOnFaulted | TaskContinuationOptions.ExecuteSynchronously,
            TaskScheduler.Default);
    }

    private sealed record InputBuildResult(
        IReadOnlyDictionary<PortKey, IReadOnlyList<DataValue>>? Inputs,
        DomainError? Error);

    private sealed record NodeRunResult(NodeExecutionResult Result, NodeExecutionTiming Timing);

    private sealed record SchedulerTermination(
        WorkflowExecutionStatus Status,
        NodeSkipReason SkipReason,
        DomainError Error);
}
