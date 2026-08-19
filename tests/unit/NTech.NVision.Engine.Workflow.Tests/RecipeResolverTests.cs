using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Recipes;
using NTech.NVision.Engine.Workflow.Registry;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Tests;

public sealed class RecipeResolverTests
{
    [Fact]
    public void ResolveAppliesDefaultThenProjectThenRecipePrecedence()
    {
        var registry = TestNodeTypes.CreateRegistry();
        WorkflowNode node = TestNodeTypes.CreateNode(
            parameters:
            [
                new(
                    TestNodeTypes.Threshold,
                    ParameterValue.FromFloatingPoint(0.7),
                    ParameterValueSource.ProjectValue),
            ]);
        WorkflowDefinition workflow = TestNodeTypes.CreateWorkflow(node);
        var recipe = CreateRecipe();
        recipe.AddOverride(new RecipeOverride(
            node.Id,
            TestNodeTypes.Threshold,
            ParameterValue.FromFloatingPoint(0.9)));

        RecipeResolutionResult result = new RecipeResolver(registry).Resolve(workflow, recipe);

        NodeParameterValue effective = result.Recipe!
            .FindNode(node.Id)!
            .Find(TestNodeTypes.Threshold)!;
        Assert.True(result.IsValid);
        Assert.Equal(0.9, effective.Value.FloatingPointValue);
        Assert.Equal(ParameterValueSource.RecipeOverride, effective.Source);
    }

    [Fact]
    public void ResolveUsesNodeDefaultWhenNoHigherPrecedenceValueExists()
    {
        var registry = TestNodeTypes.CreateRegistry();
        WorkflowNode node = TestNodeTypes.CreateNode();

        RecipeResolutionResult result = new RecipeResolver(registry)
            .Resolve(TestNodeTypes.CreateWorkflow(node), CreateRecipe());

        NodeParameterValue effective = result.Recipe!
            .FindNode(node.Id)!
            .Find(TestNodeTypes.Threshold)!;
        Assert.Equal(0.5, effective.Value.FloatingPointValue);
        Assert.Equal(ParameterValueSource.NodeDefault, effective.Source);
    }

    [Fact]
    public void ResolveRejectsUnknownTargetNode()
    {
        var registry = TestNodeTypes.CreateRegistry();
        WorkflowNode node = TestNodeTypes.CreateNode();
        var recipe = CreateRecipe();
        recipe.AddOverride(new RecipeOverride(
            NodeId.New(),
            TestNodeTypes.Threshold,
            ParameterValue.FromFloatingPoint(0.9)));

        RecipeResolutionResult result = new RecipeResolver(registry)
            .Resolve(TestNodeTypes.CreateWorkflow(node), recipe);

        Assert.False(result.IsValid);
        Assert.Null(result.Recipe);
        Assert.Contains(result.Issues, issue => issue.Error.Code == "RCP-002");
    }

    [Theory]
    [InlineData("locked", "RCP-005")]
    [InlineData("missing", "RCP-004")]
    public void ResolveRejectsNonOverridableOrUnknownParameter(string parameterKey, string expectedCode)
    {
        var registry = TestNodeTypes.CreateRegistry();
        WorkflowNode node = TestNodeTypes.CreateNode();
        var recipe = CreateRecipe();
        recipe.AddOverride(new RecipeOverride(
            node.Id,
            new ParameterKey(parameterKey),
            ParameterValue.FromFloatingPoint(0.9)));

        RecipeResolutionResult result = new RecipeResolver(registry)
            .Resolve(TestNodeTypes.CreateWorkflow(node), recipe);

        Assert.Contains(result.Issues, issue => issue.Error.Code == expectedCode);
    }

    [Fact]
    public void ResolveRejectsWrongTypeAndOutOfRangeValue()
    {
        var registry = TestNodeTypes.CreateRegistry();
        WorkflowNode first = TestNodeTypes.CreateNode();
        WorkflowNode second = TestNodeTypes.CreateNode();
        var workflow = new WorkflowDefinition(WorkflowId.New(), "Inspection");
        workflow.AddNode(first);
        workflow.AddNode(second);
        var recipe = CreateRecipe();
        recipe.AddOverride(new RecipeOverride(
            first.Id,
            TestNodeTypes.Threshold,
            ParameterValue.FromText("high")));
        recipe.AddOverride(new RecipeOverride(
            second.Id,
            TestNodeTypes.Threshold,
            ParameterValue.FromFloatingPoint(1.5)));

        RecipeResolutionResult result = new RecipeResolver(registry).Resolve(workflow, recipe);

        Assert.Contains(result.Issues, issue => issue.Error.Code == "RCP-006");
        Assert.Contains(result.Issues, issue => issue.Error.Code == "RCP-008");
    }

    [Fact]
    public void ResolveRejectsMissingRequiredEffectiveValue()
    {
        var requiredKey = new ParameterKey("requiredValue");
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(
            parameters:
            [
                new ParameterDefinition(
                    requiredKey,
                    "Required Value",
                    ParameterValueKind.Text,
                    required: true),
            ]);
        WorkflowNode node = TestNodeTypes.CreateNode(definition);

        RecipeResolutionResult result = new RecipeResolver(TestNodeTypes.CreateRegistry(definition))
            .Resolve(TestNodeTypes.CreateWorkflow(node), CreateRecipe());

        Assert.Contains(result.Issues, issue => issue.Error.Code == "RCP-007");
    }

    private static Recipe CreateRecipe() =>
        new(RecipeId.New(), "Type 01", "TYPE_01", SemanticVersion.Parse("1.0.0"));
}
