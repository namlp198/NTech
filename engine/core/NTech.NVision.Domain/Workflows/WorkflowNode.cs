using System.Collections.ObjectModel;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Common;
using NTech.NVision.Domain.Parameters;

namespace NTech.NVision.Domain.Workflows;

public sealed class WorkflowNode
{
    private readonly ReadOnlyCollection<PortDefinition> ports;
    private readonly ReadOnlyCollection<NodeParameterValue> parameters;

    public WorkflowNode(
        NodeId id,
        NodeTypeId nodeTypeId,
        string nodeTypeVersion,
        string name,
        IEnumerable<PortDefinition> ports,
        bool enabled = true,
        IEnumerable<NodeParameterValue>? parameters = null,
        NodeFailurePolicy failurePolicy = NodeFailurePolicy.AbortWorkflow,
        TimeSpan? timeout = null)
        : this(id, nodeTypeId, SemanticVersion.Parse(nodeTypeVersion), name, ports, enabled, parameters, failurePolicy, timeout)
    {
    }

    public WorkflowNode(
        NodeId id,
        NodeTypeId nodeTypeId,
        SemanticVersion nodeTypeVersion,
        string name,
        IEnumerable<PortDefinition> ports,
        bool enabled = true,
        IEnumerable<NodeParameterValue>? parameters = null,
        NodeFailurePolicy failurePolicy = NodeFailurePolicy.AbortWorkflow,
        TimeSpan? timeout = null)
    {
        if (timeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(nameof(timeout), "Node timeout must be positive when configured.");
        }

        Id = id ?? throw new ArgumentNullException(nameof(id));
        NodeTypeId = nodeTypeId ?? throw new ArgumentNullException(nameof(nodeTypeId));
        NodeTypeVersion = nodeTypeVersion ?? throw new ArgumentNullException(nameof(nodeTypeVersion));
        Name = DomainText.Require(name, nameof(name));
        Enabled = enabled;
        FailurePolicy = failurePolicy;
        Timeout = timeout;

        ArgumentNullException.ThrowIfNull(ports);
        PortDefinition[] snapshot = ports.ToArray();
        if (snapshot.Any(port => port is null))
        {
            throw new ArgumentException("Port collection must not contain null values.", nameof(ports));
        }

        bool duplicatePort = snapshot
            .GroupBy(port => (port.Direction, port.Key))
            .Any(group => group.Count() > 1);
        if (duplicatePort)
        {
            throw new ArgumentException("Port keys must be unique within each direction.", nameof(ports));
        }

        this.ports = Array.AsReadOnly(snapshot);

        NodeParameterValue[] parameterSnapshot = parameters?.ToArray() ?? [];
        if (parameterSnapshot.Any(parameter => parameter is null))
        {
            throw new ArgumentException("Parameter collection must not contain null values.", nameof(parameters));
        }

        if (parameterSnapshot.GroupBy(parameter => parameter.Key).Any(group => group.Count() > 1))
        {
            throw new ArgumentException("Parameter keys must be unique on a node.", nameof(parameters));
        }

        this.parameters = Array.AsReadOnly(parameterSnapshot);
    }

    public NodeId Id { get; }

    public NodeTypeId NodeTypeId { get; }

    public SemanticVersion NodeTypeVersion { get; }

    public string Name { get; }

    public bool Enabled { get; }

    public NodeFailurePolicy FailurePolicy { get; }

    public TimeSpan? Timeout { get; }

    public IReadOnlyList<PortDefinition> Ports => ports;

    public IReadOnlyList<NodeParameterValue> Parameters => parameters;

    public PortDefinition? FindPort(PortKey key, PortDirection direction)
    {
        ArgumentNullException.ThrowIfNull(key);
        return ports.FirstOrDefault(port => port.Direction == direction && port.Key == key);
    }

    public NodeParameterValue? FindParameter(ParameterKey key)
    {
        ArgumentNullException.ThrowIfNull(key);
        return parameters.FirstOrDefault(parameter => parameter.Key == key);
    }
}
