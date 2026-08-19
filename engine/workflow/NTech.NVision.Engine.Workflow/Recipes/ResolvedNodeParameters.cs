using System.Collections.ObjectModel;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Recipes;

public sealed class ResolvedNodeParameters
{
    private readonly ReadOnlyDictionary<ParameterKey, NodeParameterValue> values;

    public ResolvedNodeParameters(NodeId nodeId, IReadOnlyDictionary<ParameterKey, NodeParameterValue> values)
    {
        NodeId = nodeId ?? throw new ArgumentNullException(nameof(nodeId));
        ArgumentNullException.ThrowIfNull(values);
        this.values = new ReadOnlyDictionary<ParameterKey, NodeParameterValue>(
            new Dictionary<ParameterKey, NodeParameterValue>(values));
    }

    public NodeId NodeId { get; }

    public IReadOnlyDictionary<ParameterKey, NodeParameterValue> Values => values;

    public NodeParameterValue? Find(ParameterKey key)
    {
        ArgumentNullException.ThrowIfNull(key);
        return values.GetValueOrDefault(key);
    }
}
