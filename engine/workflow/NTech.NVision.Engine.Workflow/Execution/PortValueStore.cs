using System.Diagnostics.CodeAnalysis;
using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

internal sealed class PortValueStore
{
    private readonly Lock gate = new();
    private readonly Dictionary<PortAddress, DataValue> values = [];

    public void Set(PortAddress address, DataValue value)
    {
        ArgumentNullException.ThrowIfNull(address);
        ArgumentNullException.ThrowIfNull(value);
        lock (gate)
        {
            values[address] = value;
        }
    }

    public bool TryGet(PortAddress address, [NotNullWhen(true)] out DataValue? value)
    {
        ArgumentNullException.ThrowIfNull(address);
        lock (gate)
        {
            return values.TryGetValue(address, out value);
        }
    }

    public IReadOnlyDictionary<PortAddress, DataValue> Snapshot(IEnumerable<NodeId> terminalNodes)
    {
        ArgumentNullException.ThrowIfNull(terminalNodes);
        HashSet<NodeId> terminalSet = terminalNodes.ToHashSet();
        lock (gate)
        {
            return values
                .Where(pair => terminalSet.Contains(pair.Key.NodeId))
                .ToDictionary(pair => pair.Key, pair => pair.Value);
        }
    }
}
