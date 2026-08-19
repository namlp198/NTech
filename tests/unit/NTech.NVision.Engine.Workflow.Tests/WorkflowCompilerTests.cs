using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Engine.Workflow.Fakes;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Recipes;

namespace NTech.NVision.Engine.Workflow.Tests;

public sealed class WorkflowCompilerTests
{
    [Fact]
    public void CompileCreatesImmutableTopologicalPlanWithPrecomputedBindings()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();

        ExecutionPlan plan = fixture.Compile();

        Assert.Equal(4, plan.Nodes.Count);
        Assert.Equal(2, plan.EntryNodes.Count);
        Assert.Equal(fixture.Decision.Id, Assert.Single(plan.TerminalNodes));
        Assert.True(IndexOf(plan, fixture.FirstConstant.Id) < IndexOf(plan, fixture.Add.Id));
        Assert.True(IndexOf(plan, fixture.SecondConstant.Id) < IndexOf(plan, fixture.Add.Id));
        Assert.True(IndexOf(plan, fixture.Add.Id) < IndexOf(plan, fixture.Decision.Id));

        CompiledNode add = plan.Nodes.Single(node => node.NodeId == fixture.Add.Id);
        Assert.Equal(2, add.Dependencies.Count);
        Assert.Equal(2, add.InputBindings.Count);
        Assert.Single(add.OutputBindings);
        CompiledNode decision = plan.Nodes.Single(node => node.NodeId == fixture.Decision.Id);
        Assert.Equal(4, decision.BoundParameters.Values.Single().Value.FloatingPointValue);

        var nodes = Assert.IsAssignableFrom<ICollection<CompiledNode>>(plan.Nodes);
        Assert.Throws<NotSupportedException>(nodes.Clear);
        var parameters = Assert.IsAssignableFrom<IDictionary<NTech.NVision.Domain.Parameters.ParameterKey, NTech.NVision.Domain.Parameters.NodeParameterValue>>(
            decision.BoundParameters);
        Assert.Throws<NotSupportedException>(parameters.Clear);
    }

    [Fact]
    public void CompileRejectsNodeTypeWithoutExecutorFactory()
    {
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(parameters: []);
        var registry = TestNodeTypes.CreateRegistry(definition);
        WorkflowNode node = TestNodeTypes.CreateNode(definition);
        WorkflowDefinition workflow = TestNodeTypes.CreateWorkflow(node);
        var recipe = new Recipe(RecipeId.New(), "Recipe", "RECIPE", SemanticVersion.Parse("1.0.0"));
        ResolvedRecipe resolved = new RecipeResolver(registry).Resolve(workflow, recipe).Recipe!;

        WorkflowCompilationResult result = new WorkflowCompiler(registry).Compile(workflow, resolved);

        Assert.False(result.IsSuccess);
        Assert.Null(result.Plan);
        Assert.Contains(result.Issues, issue => issue.Error.Code == "CMP-002");
    }

    [Fact]
    public void CompileRejectsResolvedRecipeFromDifferentNodeSet()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();
        var emptyParameters = new Dictionary<NodeId, ResolvedNodeParameters>();
        var unrelated = new ResolvedRecipe(
            fixture.Recipe.Id,
            fixture.Recipe.Version,
            emptyParameters);

        WorkflowCompilationResult result = new WorkflowCompiler(fixture.Registry)
            .Compile(fixture.Workflow, unrelated);

        Assert.False(result.IsSuccess);
        Assert.Contains(result.Issues, issue => issue.Error.Code == "CMP-003");
    }

    [Fact]
    public void CompileUsesStableOrderForSameGraph()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();

        ExecutionPlan first = fixture.Compile();
        ExecutionPlan second = fixture.Compile();

        Assert.NotEqual(first.Id, second.Id);
        Assert.Equal(
            first.Nodes.Select(node => node.NodeId),
            second.Nodes.Select(node => node.NodeId));
    }

    [Fact]
    public void CompileRejectsManuallyConstructedIncompatibleBoundParameter()
    {
        FakeWorkflowFixture fixture = FakeWorkflowFixture.Create();
        ResolvedRecipe valid = new RecipeResolver(fixture.Registry)
            .Resolve(fixture.Workflow, fixture.Recipe)
            .Recipe!;
        Dictionary<NodeId, ResolvedNodeParameters> nodes = valid.EffectiveNodeParameters
            .ToDictionary(pair => pair.Key, pair => pair.Value);
        ResolvedNodeParameters decisionParameters = nodes[fixture.Decision.Id];
        Dictionary<ParameterKey, NodeParameterValue> invalidValues = decisionParameters.Values
            .ToDictionary(pair => pair.Key, pair => pair.Value);
        invalidValues[FakeNodeCatalog.ThresholdParameter] = new NodeParameterValue(
            FakeNodeCatalog.ThresholdParameter,
            ParameterValue.FromText("four"),
            ParameterValueSource.RecipeOverride);
        nodes[fixture.Decision.Id] = new ResolvedNodeParameters(fixture.Decision.Id, invalidValues);
        var invalid = new ResolvedRecipe(valid.RecipeId, valid.Version, nodes);

        WorkflowCompilationResult result = new WorkflowCompiler(fixture.Registry)
            .Compile(fixture.Workflow, invalid);

        Assert.Contains(result.Issues, issue => issue.Error.Code == "CMP-007");
    }

    private static int IndexOf(ExecutionPlan plan, NodeId nodeId) =>
        plan.Nodes.Select((node, index) => (node, index)).Single(item => item.node.NodeId == nodeId).index;
}
