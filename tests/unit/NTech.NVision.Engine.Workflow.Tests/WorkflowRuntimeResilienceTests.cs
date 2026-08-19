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

public sealed class WorkflowRuntimeResilienceTests
{
    private static readonly PortKey ValuePort = new("value");

    [Fact]
    public async Task AbortWorkflowFailureStopsExecutionAndMarksRemainingNodesSkipped()
    {
        NodeTypeDefinition failingType = Definition(
            "abort-failure",
            [Output()],
            _ => ValueTask.FromResult(NodeExecutionResult.Failure(new DomainError("TST-001", "Expected failure."))));
        NodeTypeDefinition dependentType = Definition(
            "abort-dependent",
            [Input()],
            _ => ValueTask.FromResult(NodeExecutionResult.Success()));
        WorkflowNode failing = Node(failingType, NodeFailurePolicy.AbortWorkflow);
        WorkflowNode dependent = Node(dependentType);
        WorkflowDefinition workflow = Workflow([failing, dependent]);
        Connect(workflow, failing, dependent);

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime()
            .ExecuteAsync(Compile(workflow, failingType, dependentType));

        Assert.Equal(WorkflowExecutionStatus.Failed, result.Status);
        Assert.Equal(NodeExecutionStatus.Failed, Find(result, failing).Result.Status);
        Assert.Equal(NodeExecutionStatus.Skipped, Find(result, dependent).Result.Status);
        Assert.Equal(NodeSkipReason.WorkflowAborted, Find(result, dependent).Result.SkipReason);
        WorkflowExecutionError error = Assert.Single(result.Errors);
        Assert.False(error.Recoverable);
        Assert.Equal(failing.Id, error.NodeId);
    }

    [Fact]
    public async Task ContinueFailureRunsIndependentNodeAndSkipsFailedDependencyBranch()
    {
        int independentExecutions = 0;
        NodeTypeDefinition failingType = Definition(
            "continue-failure",
            [Output()],
            _ => ValueTask.FromResult(NodeExecutionResult.Failure(new DomainError("TST-002", "Recoverable failure."))));
        NodeTypeDefinition dependentType = Definition(
            "continue-dependent",
            [Input()],
            _ => ValueTask.FromResult(NodeExecutionResult.Success()));
        NodeTypeDefinition independentType = Definition(
            "continue-independent",
            [],
            _ =>
            {
                independentExecutions++;
                return ValueTask.FromResult(NodeExecutionResult.Success());
            });
        WorkflowNode failing = Node(failingType, NodeFailurePolicy.Continue);
        WorkflowNode dependent = Node(dependentType);
        WorkflowNode independent = Node(independentType);
        WorkflowDefinition workflow = Workflow([failing, dependent, independent]);
        Connect(workflow, failing, dependent);

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime()
            .ExecuteAsync(Compile(workflow, failingType, dependentType, independentType));

        Assert.True(result.IsSuccess);
        Assert.Null(result.Error);
        Assert.Equal(1, independentExecutions);
        Assert.Equal(NodeExecutionStatus.Succeeded, Find(result, independent).Result.Status);
        Assert.Equal(NodeSkipReason.DependencyFailed, Find(result, dependent).Result.SkipReason);
        WorkflowExecutionError error = Assert.Single(result.Errors);
        Assert.True(error.Recoverable);
        Assert.Equal(failing.Id, error.NodeId);
    }

    [Fact]
    public async Task NodeTimeoutReturnsEvenWhenExecutorIgnoresCancellation()
    {
        NodeTypeDefinition slowType = Definition(
            "node-timeout",
            [],
            async _ =>
            {
                await Task.Delay(TimeSpan.FromSeconds(5), CancellationToken.None);
                return NodeExecutionResult.Success();
            });
        WorkflowNode slow = Node(slowType, timeout: TimeSpan.FromMilliseconds(40));
        WorkflowDefinition workflow = Workflow([slow]);

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime()
            .ExecuteAsync(Compile(workflow, slowType));

        Assert.Equal(WorkflowExecutionStatus.TimedOut, result.Status);
        Assert.Equal(NodeExecutionStatus.TimedOut, Assert.Single(result.NodeResults).Result.Status);
        Assert.Equal("EXE-008", result.Error?.Code);
        Assert.Equal(WorkflowErrorCategory.Timeout, Assert.Single(result.Errors).Category);
    }

    [Fact]
    public async Task WorkflowTimeoutTakesPrecedenceOverNodeFailurePolicy()
    {
        NodeTypeDefinition slowType = Definition(
            "workflow-timeout",
            [],
            async _ =>
            {
                await Task.Delay(TimeSpan.FromSeconds(5), CancellationToken.None);
                return NodeExecutionResult.Success();
            });
        WorkflowNode slow = Node(slowType, NodeFailurePolicy.Continue);
        WorkflowDefinition workflow = Workflow([slow], TimeSpan.FromMilliseconds(40));

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime()
            .ExecuteAsync(Compile(workflow, slowType));

        Assert.Equal(WorkflowExecutionStatus.TimedOut, result.Status);
        Assert.Equal("EXE-009", result.Error?.Code);
        Assert.False(Assert.Single(result.Errors).Recoverable);
    }

    [Fact]
    public async Task ExternalCancellationReturnsWhenExecutorIgnoresCancellation()
    {
        var started = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var neverCompletes = new TaskCompletionSource<NodeExecutionResult>(TaskCreationOptions.RunContinuationsAsynchronously);
        NodeTypeDefinition blockingType = Definition(
            "external-cancellation",
            [],
            _ =>
            {
                started.SetResult();
                return new ValueTask<NodeExecutionResult>(neverCompletes.Task);
            });
        WorkflowNode blocking = Node(blockingType);
        WorkflowDefinition workflow = Workflow([blocking]);
        using var cancellationSource = new CancellationTokenSource();

        Task<WorkflowExecutionResult> execution = new SequentialWorkflowRuntime()
            .ExecuteAsync(Compile(workflow, blockingType), cancellationSource.Token)
            .AsTask();
        await started.Task.WaitAsync(TimeSpan.FromSeconds(2));
        cancellationSource.Cancel();
        WorkflowExecutionResult result = await execution.WaitAsync(TimeSpan.FromSeconds(2));

        Assert.Equal(WorkflowExecutionStatus.Cancelled, result.Status);
        Assert.Equal(NodeExecutionStatus.Cancelled, Assert.Single(result.NodeResults).Result.Status);
        Assert.Equal("EXE-004", result.Error?.Code);
    }

    [Fact]
    public async Task ExecutionEmitsOrderedTraceAndMonotonicTiming()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();
        ExecutionPlan plan = fixture.Compile();
        var traceSink = new RecordingTraceSink();

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime(traceSink: traceSink)
            .ExecuteAsync(plan);

        Assert.True(result.IsSuccess);
        Assert.NotNull(result.Timing);
        Assert.True(result.Timing.Duration >= TimeSpan.Zero);
        Assert.All(result.NodeResults, record =>
        {
            Assert.NotNull(record.Timing);
            Assert.True(record.Timing.Duration >= TimeSpan.Zero);
        });
        Assert.Equal(ExecutionTraceEventKind.WorkflowStarted, traceSink.Events[0].Kind);
        Assert.Equal(ExecutionTraceEventKind.WorkflowCompleted, traceSink.Events[^1].Kind);
        Assert.Equal(2 + (2 * plan.Nodes.Count), traceSink.Events.Count);
        Assert.Equal(
            result.NodeResults.Select(record => record.NodeId),
            traceSink.Events
                .Where(item => item.Kind == ExecutionTraceEventKind.NodeCompleted)
                .Select(item => item.NodeId));
        Assert.All(traceSink.Events, item => Assert.Equal(result.ExecutionId, item.ExecutionId));
    }

    [Fact]
    public async Task TraceSinkFailureBecomesWarningWithoutFailingWorkflow()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime(traceSink: new ThrowingTraceSink())
            .ExecuteAsync(fixture.Compile());

        Assert.True(result.IsSuccess);
        Assert.Equal("TRC-001", Assert.Single(result.Warnings).Code);
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
        NodeFailurePolicy failurePolicy = NodeFailurePolicy.AbortWorkflow,
        TimeSpan? timeout = null) =>
        new(
            NodeId.New(),
            definition.Id,
            definition.Version,
            definition.DisplayName,
            definition.Ports,
            failurePolicy: failurePolicy,
            timeout: timeout);

    private static WorkflowDefinition Workflow(
        IEnumerable<WorkflowNode> nodes,
        TimeSpan? timeout = null)
    {
        var workflow = new WorkflowDefinition(WorkflowId.New(), "Runtime resilience", timeout: timeout);
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
            ValuePort,
            target.Id,
            ValuePort)).IsSuccess);

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
            "Runtime resilience",
            "RUNTIME-RESILIENCE",
            SemanticVersion.Parse("1.0.0"));
        RecipeResolutionResult resolution = new RecipeResolver(registry).Resolve(workflow, recipe);
        Assert.True(resolution.IsValid);
        WorkflowCompilationResult compilation = new WorkflowCompiler(registry).Compile(workflow, resolution.Recipe!);
        Assert.True(compilation.IsSuccess);
        return compilation.Plan!;
    }

    private static PortDefinition Output() =>
        new(ValuePort, "Value", PortDirection.Output, PortDataType.Boolean, required: true);

    private static PortDefinition Input() =>
        new(ValuePort, "Value", PortDirection.Input, PortDataType.Boolean, required: true);

    private sealed class DelegateExecutor(
        Func<CancellationToken, ValueTask<NodeExecutionResult>> execute) : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken) => execute(cancellationToken);
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

    private sealed class ThrowingTraceSink : IExecutionTraceSink
    {
        public ValueTask WriteAsync(
            ExecutionTraceEvent traceEvent,
            CancellationToken cancellationToken = default) =>
            throw new InvalidOperationException("Trace storage unavailable.");
    }
}
