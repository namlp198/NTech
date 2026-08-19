using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Domain.Tests;

public sealed class RecipeTests
{
    [Fact]
    public void AddOverrideRejectsDuplicateNodeParameterPath()
    {
        var recipe = new Recipe(RecipeId.New(), "Type 01", "TYPE_01", SemanticVersion.Parse("1.0.0"));
        NodeId nodeId = NodeId.New();
        var key = new ParameterKey("confidenceThreshold");
        recipe.AddOverride(new RecipeOverride(nodeId, key, ParameterValue.FromFloatingPoint(0.8)));

        var result = recipe.AddOverride(
            new RecipeOverride(nodeId, key, ParameterValue.FromFloatingPoint(0.9)));

        Assert.True(result.IsFailure);
        Assert.Equal("RCP-001", result.Error?.Code);
        Assert.Single(recipe.Overrides);
    }

    [Fact]
    public void ParameterValueRejectsNonFiniteFloatingPoint()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            ParameterValue.FromFloatingPoint(double.NaN));
    }

    [Fact]
    public void WorkflowNodeRejectsDuplicateParameterKeys()
    {
        var key = new ParameterKey("threshold");
        NodeParameterValue[] parameters =
        [
            new(key, ParameterValue.FromInteger(1), ParameterValueSource.ProjectValue),
            new(key, ParameterValue.FromInteger(2), ParameterValueSource.ProjectValue),
        ];

        Assert.Throws<ArgumentException>(() => new WorkflowNode(
            NodeId.New(),
            new NodeTypeId("nvision.test.node"),
            "1.0.0",
            "Node",
            [],
            parameters: parameters));
    }
}
