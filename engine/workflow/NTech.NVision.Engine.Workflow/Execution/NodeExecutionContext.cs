using System.Collections.ObjectModel;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed class NodeExecutionContext
{
    private readonly ReadOnlyDictionary<PortKey, IReadOnlyList<DataValue>> inputs;
    private readonly ReadOnlyDictionary<ParameterKey, NodeParameterValue> parameters;

    public NodeExecutionContext(
        NodeId nodeId,
        IReadOnlyDictionary<PortKey, IReadOnlyList<DataValue>> inputs,
        IReadOnlyDictionary<ParameterKey, NodeParameterValue> parameters)
    {
        NodeId = nodeId ?? throw new ArgumentNullException(nameof(nodeId));
        ArgumentNullException.ThrowIfNull(inputs);
        ArgumentNullException.ThrowIfNull(parameters);
        this.inputs = new ReadOnlyDictionary<PortKey, IReadOnlyList<DataValue>>(
            inputs.ToDictionary(
                pair => pair.Key,
                pair => (IReadOnlyList<DataValue>)Array.AsReadOnly(pair.Value.ToArray())));
        this.parameters = new ReadOnlyDictionary<ParameterKey, NodeParameterValue>(
            new Dictionary<ParameterKey, NodeParameterValue>(parameters));
    }

    public NodeId NodeId { get; }

    public IReadOnlyDictionary<PortKey, IReadOnlyList<DataValue>> Inputs => inputs;

    public IReadOnlyDictionary<ParameterKey, NodeParameterValue> Parameters => parameters;

    public TypedDataValue<T> GetSingleInput<T>(PortKey key, PortDataType expectedType)
    {
        ArgumentNullException.ThrowIfNull(key);
        ArgumentNullException.ThrowIfNull(expectedType);
        if (!inputs.TryGetValue(key, out IReadOnlyList<DataValue>? values) || values.Count != 1)
        {
            throw new InvalidOperationException($"Input port '{key}' must contain exactly one value.");
        }

        if (values[0].DataType != expectedType || values[0] is not TypedDataValue<T> typedValue)
        {
            throw new InvalidOperationException($"Input port '{key}' does not contain the expected data type '{expectedType}'.");
        }

        return typedValue;
    }

    public NodeParameterValue GetRequiredParameter(ParameterKey key)
    {
        ArgumentNullException.ThrowIfNull(key);
        return parameters.TryGetValue(key, out NodeParameterValue? value)
            ? value
            : throw new InvalidOperationException($"Required parameter '{key}' is not bound.");
    }
}
