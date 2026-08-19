using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Domain.Recipes;

public sealed record RecipeOverride
{
    public RecipeOverride(NodeId targetNodeId, ParameterKey parameterKey, ParameterValue value)
    {
        TargetNodeId = targetNodeId ?? throw new ArgumentNullException(nameof(targetNodeId));
        ParameterKey = parameterKey ?? throw new ArgumentNullException(nameof(parameterKey));
        Value = value ?? throw new ArgumentNullException(nameof(value));
    }

    public NodeId TargetNodeId { get; }

    public ParameterKey ParameterKey { get; }

    public ParameterValue Value { get; }
}
