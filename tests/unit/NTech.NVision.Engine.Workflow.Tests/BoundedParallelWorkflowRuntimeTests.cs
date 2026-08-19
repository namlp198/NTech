using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Engine.Workflow.Execution;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Recipes;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Workflow.Tests;

public sealed class BoundedParallelWorkflowRuntimeTests
{
    private static readonly PortKey InputPort = new("input");
    private static readonly PortKey OutputPort = new("output");

    [Fact]
    public void ConstructorRejectsNonPositiveConcurrency()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new BoundedParallelWorkflowRuntime(0));
    }

    [Fact]
    public async Task IndependentNodesOverlapWithoutExceedingConfiguredLimit()
    {
        var probe = new ConcurrencyProbe(expectedOverlap: 2);
        NodeTypeDefinition type = Definition(
            "bounded-independent",
            [],
            cancellationToken => probe.ExecuteAsync(cancellationToken));
        WorkflowNode[] nodes = Enumerable.Range(0, 4).Select(_ => Node(type)).ToArray();
        WorkflowDefinition workflow = Workflow(nodes);
        ExecutionPlan plan = Compile(workflow, type);
        var trace = new RecordingTraceSink();
        using var runtime = new BoundedParallelWorkflowRuntime(2, traceSink: trace);

        Task<WorkflowExecutionResult> execution = runtime.ExecuteAsync(plan).AsTask();
        await probe.ExpectedOverlapReached.WaitAsync(TimeSpan.FromSeconds(2));
        Assert.Equal(2, probe.MaximumConcurrency);
        probe.Release();
        WorkflowExecutionResult result = await execution.WaitAsync(TimeSpan.FromSeconds(2));

        Assert.True(result.IsSuccess);
        Assert.Equal(2, runtime.MaxConcurrency);
        Assert.Equal(2, probe.MaximumConcurrency);
        Assert.Equal(plan.Nodes.Select(node => node.NodeId), result.NodeResults.Select(record => record.NodeId));
        Assert.Equal(
            plan.Nodes.Select(node => node.NodeId),
            trace.Events
                .Where(traceEvent => traceEvent.Kind == ExecutionTraceEventKind.NodeStarted)
                .Select(traceEvent => traceEvent.NodeId));
    }

    [Fact]
    public async Task SynchronousExecutorsCanRunOnParallelWorkers()
    {
        using var barrier = new Barrier(participantCount: 2);
        NodeTypeDefinition type = Definition(
            "bounded-synchronous",
            [],
            _ => barrier.SignalAndWait(TimeSpan.FromSeconds(2), CancellationToken.None)
                ? ValueTask.FromResult(NodeExecutionResult.Success())
                : ValueTask.FromResult(NodeExecutionResult.Failure(
                    new DomainError("TST-100", "Synchronous workers did not overlap."))));
        WorkflowNode[] nodes = [Node(type), Node(type)];
        using var runtime = new BoundedParallelWorkflowRuntime(2);

        WorkflowExecutionResult result = await runtime
            .ExecuteAsync(Compile(Workflow(nodes), type))
            .AsTask()
            .WaitAsync(TimeSpan.FromSeconds(3));

        Assert.True(result.IsSuccess);
        Assert.All(result.NodeResults, record => Assert.Equal(NodeExecutionStatus.Succeeded, record.Result.Status));
    }

    [Fact]
    public async Task JoinRunsOnlyAfterParallelDependenciesComplete()
    {
        var branches = new ConcurrencyProbe(expectedOverlap: 2);
        int sourceExecutions = 0;
        int joinExecutions = 0;
        NodeTypeDefinition sourceType = Definition(
            "parallel-source",
            [Output(allowsMultiple: true)],
            _ =>
            {
                Interlocked.Increment(ref sourceExecutions);
                return ValueTask.FromResult(SuccessWithOutput());
            });
        NodeTypeDefinition branchType = Definition(
            "parallel-branch",
            [Input(), Output()],
            async cancellationToken =>
            {
                await branches.ExecuteAsync(cancellationToken);
                return SuccessWithOutput();
            });
        NodeTypeDefinition joinType = Definition(
            "parallel-join",
            [Input(allowsMultiple: true)],
            _ =>
            {
                Assert.Equal(0, branches.ActiveExecutions);
                Interlocked.Increment(ref joinExecutions);
                return ValueTask.FromResult(NodeExecutionResult.Success());
            });
        WorkflowNode source = Node(sourceType);
        WorkflowNode firstBranch = Node(branchType);
        WorkflowNode secondBranch = Node(branchType);
        WorkflowNode join = Node(joinType);
        WorkflowDefinition workflow = Workflow([source, firstBranch, secondBranch, join]);
        Connect(workflow, source, firstBranch);
        Connect(workflow, source, secondBranch);
        Connect(workflow, firstBranch, join);
        Connect(workflow, secondBranch, join);
        using var runtime = new BoundedParallelWorkflowRuntime(2);

        Task<WorkflowExecutionResult> execution = runtime
            .ExecuteAsync(Compile(workflow, sourceType, branchType, joinType))
            .AsTask();
        await branches.ExpectedOverlapReached.WaitAsync(TimeSpan.FromSeconds(2));
        Assert.Equal(1, sourceExecutions);
        Assert.Equal(0, joinExecutions);
        branches.Release();
        WorkflowExecutionResult result = await execution.WaitAsync(TimeSpan.FromSeconds(2));

        Assert.True(result.IsSuccess);
        Assert.Equal(2, branches.MaximumConcurrency);
        Assert.Equal(1, joinExecutions);
    }

    [Fact]
    public async Task FatalFailureCancelsRunningSiblingAndSkipsPendingDependent()
    {
        var failureStarted = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var siblingStarted = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var releaseFailure = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var siblingNeverCompletes = new TaskCompletionSource<NodeExecutionResult>(
            TaskCreationOptions.RunContinuationsAsynchronously);
        int dependentExecutions = 0;
        NodeTypeDefinition failingType = Definition(
            "parallel-fatal",
            [],
            async cancellationToken =>
            {
                failureStarted.SetResult();
                await releaseFailure.Task.WaitAsync(cancellationToken);
                return NodeExecutionResult.Failure(new DomainError("TST-101", "Expected fatal failure."));
            });
        NodeTypeDefinition siblingType = Definition(
            "parallel-running-sibling",
            [Output()],
            _ =>
            {
                siblingStarted.SetResult();
                return new ValueTask<NodeExecutionResult>(siblingNeverCompletes.Task);
            });
        NodeTypeDefinition dependentType = Definition(
            "parallel-pending-dependent",
            [Input()],
            _ =>
            {
                Interlocked.Increment(ref dependentExecutions);
                return ValueTask.FromResult(NodeExecutionResult.Success());
            });
        WorkflowNode failing = Node(failingType, NodeFailurePolicy.AbortWorkflow);
        WorkflowNode sibling = Node(siblingType);
        WorkflowNode dependent = Node(dependentType);
        WorkflowDefinition workflow = Workflow([failing, sibling, dependent]);
        Connect(workflow, sibling, dependent);
        using var runtime = new BoundedParallelWorkflowRuntime(2);

        Task<WorkflowExecutionResult> execution = runtime
            .ExecuteAsync(Compile(workflow, failingType, siblingType, dependentType))
            .AsTask();
        await Task.WhenAll(failureStarted.Task, siblingStarted.Task).WaitAsync(TimeSpan.FromSeconds(2));
        releaseFailure.SetResult();
        WorkflowExecutionResult result = await execution.WaitAsync(TimeSpan.FromSeconds(2));

        Assert.Equal(WorkflowExecutionStatus.Failed, result.Status);
        Assert.Equal(NodeExecutionStatus.Failed, Find(result, failing).Result.Status);
        Assert.Equal(NodeExecutionStatus.Cancelled, Find(result, sibling).Result.Status);
        Assert.Equal(NodeExecutionStatus.Skipped, Find(result, dependent).Result.Status);
        Assert.Equal(NodeSkipReason.WorkflowAborted, Find(result, dependent).Result.SkipReason);
        Assert.Equal(0, dependentExecutions);
        Assert.Equal("TST-101", result.Error?.Code);
    }

    [Fact]
    public async Task WorkflowTimeoutCancelsAllRunningParallelNodes()
    {
        var neverCompletes = new TaskCompletionSource<NodeExecutionResult>(
            TaskCreationOptions.RunContinuationsAsynchronously);
        NodeTypeDefinition type = Definition(
            "parallel-workflow-timeout",
            [],
            _ => new ValueTask<NodeExecutionResult>(neverCompletes.Task));
        WorkflowNode[] nodes = [Node(type), Node(type)];
        WorkflowDefinition workflow = Workflow(nodes, timeout: TimeSpan.FromMilliseconds(40));
        using var runtime = new BoundedParallelWorkflowRuntime(2);

        WorkflowExecutionResult result = await runtime
            .ExecuteAsync(Compile(workflow, type))
            .AsTask()
            .WaitAsync(TimeSpan.FromSeconds(2));

        Assert.Equal(WorkflowExecutionStatus.TimedOut, result.Status);
        Assert.All(result.NodeResults, record => Assert.Equal(NodeExecutionStatus.TimedOut, record.Result.Status));
        Assert.Equal("EXE-009", result.Error?.Code);
    }

    [Fact]
    public async Task ContinueFailureSkipsItsBranchWhileIndependentWorkCompletes()
    {
        var bothStarted = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        int startedCount = 0;
        int dependentExecutions = 0;
        NodeTypeDefinition failingType = Definition(
            "parallel-continue-failure",
            [Output()],
            async cancellationToken =>
            {
                SignalStarted();
                await release.Task.WaitAsync(cancellationToken);
                return NodeExecutionResult.Failure(new DomainError("TST-102", "Recoverable failure."));
            });
        NodeTypeDefinition independentType = Definition(
            "parallel-continue-independent",
            [],
            async cancellationToken =>
            {
                SignalStarted();
                await release.Task.WaitAsync(cancellationToken);
                return NodeExecutionResult.Success();
            });
        NodeTypeDefinition dependentType = Definition(
            "parallel-continue-dependent",
            [Input()],
            _ =>
            {
                Interlocked.Increment(ref dependentExecutions);
                return ValueTask.FromResult(NodeExecutionResult.Success());
            });
        WorkflowNode failing = Node(failingType, NodeFailurePolicy.Continue);
        WorkflowNode independent = Node(independentType);
        WorkflowNode dependent = Node(dependentType);
        WorkflowDefinition workflow = Workflow([failing, independent, dependent]);
        Connect(workflow, failing, dependent);
        using var runtime = new BoundedParallelWorkflowRuntime(2);

        Task<WorkflowExecutionResult> execution = runtime
            .ExecuteAsync(Compile(workflow, failingType, independentType, dependentType))
            .AsTask();
        await bothStarted.Task.WaitAsync(TimeSpan.FromSeconds(2));
        release.SetResult();
        WorkflowExecutionResult result = await execution.WaitAsync(TimeSpan.FromSeconds(2));

        Assert.True(result.IsSuccess);
        Assert.Equal(NodeExecutionStatus.Succeeded, Find(result, independent).Result.Status);
        Assert.Equal(NodeSkipReason.DependencyFailed, Find(result, dependent).Result.SkipReason);
        Assert.Equal(0, dependentExecutions);
        Assert.True(Assert.Single(result.Errors).Recoverable);

        void SignalStarted()
        {
            if (Interlocked.Increment(ref startedCount) == 2)
            {
                bothStarted.SetResult();
            }
        }
    }

    private static NodeExecutionRecord Find(WorkflowExecutionResult result, WorkflowNode node) =>
        result.NodeResults.Single(record => record.NodeId == node.Id);

    private static NodeTypeDefinition Definition(
        string suffix,
        IEnumerable<PortDefinition> ports,
        Func<CancellationToken, ValueTask<NodeExecutionResult>> execute) =>
        new(
            new NodeTypeId($"nvision.test.{suffix}"),
            SemanticVersion.Parse("1.0.0"),
            suffix,
            "Testing",
            ports,
            executionCharacteristics: new NodeExecutionCharacteristics(NodeExecutionLifetime.Stateless),
            executorFactory: new DelegateNodeExecutorFactory(() => new DelegateExecutor(execute)));

    private static WorkflowNode Node(
        NodeTypeDefinition definition,
        NodeFailurePolicy failurePolicy = NodeFailurePolicy.AbortWorkflow) =>
        new(
            NodeId.New(),
            definition.Id,
            definition.Version,
            definition.DisplayName,
            definition.Ports,
            failurePolicy: failurePolicy);

    private static WorkflowDefinition Workflow(
        IEnumerable<WorkflowNode> nodes,
        TimeSpan? timeout = null)
    {
        var workflow = new WorkflowDefinition(
            WorkflowId.New(),
            "Bounded parallel runtime",
            timeout: timeout);
        foreach (WorkflowNode node in nodes)
        {
            Assert.True(workflow.AddNode(node).IsSuccess);
        }

        return workflow;
    }

    private static void Connect(WorkflowDefinition workflow, WorkflowNode source, WorkflowNode target) =>
        Assert.True(workflow.AddConnection(new WorkflowConnection(
            ConnectionId.New(),
            source.Id,
            OutputPort,
            target.Id,
            InputPort)).IsSuccess);

    private static ExecutionPlan Compile(
        WorkflowDefinition workflow,
        params NodeTypeDefinition[] definitions)
    {
        var registry = new NodeTypeRegistry();
        foreach (NodeTypeDefinition definition in definitions)
        {
            registry.Register(definition);
        }

        var recipe = new Recipe(
            RecipeId.New(),
            "Bounded parallel runtime",
            "BOUNDED-PARALLEL",
            SemanticVersion.Parse("1.0.0"));
        RecipeResolutionResult resolution = new RecipeResolver(registry).Resolve(workflow, recipe);
        Assert.True(resolution.IsValid);
        WorkflowCompilationResult compilation = new WorkflowCompiler(registry).Compile(workflow, resolution.Recipe!);
        Assert.True(compilation.IsSuccess);
        return compilation.Plan!;
    }

    private static NodeExecutionResult SuccessWithOutput() =>
        NodeExecutionResult.Success(new Dictionary<PortKey, DataValue>
        {
            [OutputPort] = DataValue.FromBoolean(true),
        });

    private static PortDefinition Input(bool allowsMultiple = false) =>
        new(
            InputPort,
            "Input",
            PortDirection.Input,
            PortDataType.Boolean,
            required: true,
            multiConnectionAllowed: allowsMultiple);

    private static PortDefinition Output(bool allowsMultiple = false) =>
        new(
            OutputPort,
            "Output",
            PortDirection.Output,
            PortDataType.Boolean,
            required: true,
            multiConnectionAllowed: allowsMultiple);

    private sealed class DelegateExecutor(
        Func<CancellationToken, ValueTask<NodeExecutionResult>> execute) : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken) => execute(cancellationToken);
    }

    private sealed class ConcurrencyProbe(int expectedOverlap)
    {
        private readonly TaskCompletionSource expectedOverlapReached = new(
            TaskCreationOptions.RunContinuationsAsynchronously);
        private readonly TaskCompletionSource release = new(TaskCreationOptions.RunContinuationsAsynchronously);
        private int activeExecutions;
        private int maximumConcurrency;

        public Task ExpectedOverlapReached => expectedOverlapReached.Task;

        public int ActiveExecutions => Volatile.Read(ref activeExecutions);

        public int MaximumConcurrency => Volatile.Read(ref maximumConcurrency);

        public async ValueTask<NodeExecutionResult> ExecuteAsync(CancellationToken cancellationToken)
        {
            int active = Interlocked.Increment(ref activeExecutions);
            UpdateMaximum(active);
            if (active >= expectedOverlap)
            {
                expectedOverlapReached.TrySetResult();
            }

            try
            {
                await release.Task.WaitAsync(cancellationToken);
                return NodeExecutionResult.Success();
            }
            finally
            {
                Interlocked.Decrement(ref activeExecutions);
            }
        }

        public void Release() => release.TrySetResult();

        private void UpdateMaximum(int candidate)
        {
            int observed = Volatile.Read(ref maximumConcurrency);
            while (candidate > observed)
            {
                int original = Interlocked.CompareExchange(ref maximumConcurrency, candidate, observed);
                if (original == observed)
                {
                    return;
                }

                observed = original;
            }
        }
    }

    private sealed class RecordingTraceSink : IExecutionTraceSink
    {
        public List<ExecutionTraceEvent> Events { get; } = [];

        public ValueTask WriteAsync(
            ExecutionTraceEvent traceEvent,
            CancellationToken cancellationToken = default)
        {
            Events.Add(traceEvent);
            return ValueTask.CompletedTask;
        }
    }
}
