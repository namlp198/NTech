using System.Collections.ObjectModel;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Recipes;

public sealed class ResolvedRecipe
{
    private readonly ReadOnlyDictionary<NodeId, ResolvedNodeParameters> nodeParameters;

    public ResolvedRecipe(
        RecipeId recipeId,
        SemanticVersion version,
        IReadOnlyDictionary<NodeId, ResolvedNodeParameters> nodeParameters)
    {
        RecipeId = recipeId ?? throw new ArgumentNullException(nameof(recipeId));
        Version = version ?? throw new ArgumentNullException(nameof(version));
        ArgumentNullException.ThrowIfNull(nodeParameters);
        this.nodeParameters = new ReadOnlyDictionary<NodeId, ResolvedNodeParameters>(
            new Dictionary<NodeId, ResolvedNodeParameters>(nodeParameters));
    }

    public RecipeId RecipeId { get; }

    public SemanticVersion Version { get; }

    public IReadOnlyDictionary<NodeId, ResolvedNodeParameters> EffectiveNodeParameters => nodeParameters;

    public ResolvedNodeParameters? FindNode(NodeId nodeId)
    {
        ArgumentNullException.ThrowIfNull(nodeId);
        return nodeParameters.GetValueOrDefault(nodeId);
    }
}
