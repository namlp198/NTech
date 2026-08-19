using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Engine.Workflow.Execution;
using NTech.NVision.Engine.Workflow.Fakes;

namespace NTech.NVision.Engine.Workflow.Tests;

public sealed class SequentialWorkflowRuntimeTests
{
    [Fact]
    public async Task ExecuteAsyncRunsCompiledNodesSequentiallyAndReturnsTerminalOutput()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();
        ExecutionPlan plan = fixture.Compile();

        WorkflowExecutionResult result = await new SequentialWorkflowRuntime().ExecuteAsync(plan);

        Assert.True(result.IsSuccess);
        Assert.Equal(plan.Nodes.Select(node => node.NodeId), result.NodeResults.Select(node => node.NodeId));
        var outputAddress = new PortAddress(fixture.Decision.Id, FakeNodeCatalog.DecisionPort);
        var decision = Assert.IsType<TypedDataValue<bool>>(result.Outputs[outputAddress]);
        Assert.True(decision.Value);
    }

    [Fact]
    public async Task ExecuteAsyncRepeatedlyProducesStableResultFromSamePlan()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();
        ExecutionPlan plan = fixture.Compile();
        var runtime = new SequentialWorkflowRuntime();

        WorkflowExecutionResult first = await runtime.ExecuteAsync(plan);
        WorkflowExecutionResult second = await runtime.ExecuteAsync(plan);

        var outputAddress = new PortAddress(fixture.Decision.Id, FakeNodeCatalog.DecisionPort);
        Assert.NotEqual(first.ExecutionId, second.ExecutionId);
        Assert.Equal(first.Status, second.Status);
        Assert.Equal(first.Outputs[outputAddress], second.Outputs[outputAddress]);
    }
}
