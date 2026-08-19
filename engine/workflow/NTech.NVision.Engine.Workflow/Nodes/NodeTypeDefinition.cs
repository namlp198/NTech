using System.Collections.ObjectModel;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Execution;

namespace NTech.NVision.Engine.Workflow.Nodes;

public sealed class NodeTypeDefinition
{
    private readonly ReadOnlyCollection<ParameterDefinition> parameters;
    private readonly ReadOnlyCollection<PortDefinition> ports;

    public NodeTypeDefinition(
        NodeTypeId id,
        SemanticVersion version,
        string displayName,
        string category,
        IEnumerable<PortDefinition> ports,
        IEnumerable<ParameterDefinition>? parameters = null,
        NodeExecutionCharacteristics? executionCharacteristics = null,
        NodeTypeOrigin? origin = null,
        INodeExecutorFactory? executorFactory = null)
    {
        if (string.IsNullOrWhiteSpace(displayName))
        {
            throw new ArgumentException("Node type display name is required.", nameof(displayName));
        }

        if (string.IsNullOrWhiteSpace(category))
        {
            throw new ArgumentException("Node type category is required.", nameof(category));
        }

        ArgumentNullException.ThrowIfNull(ports);
        PortDefinition[] portSnapshot = ports.ToArray();
        ParameterDefinition[] parameterSnapshot = parameters?.ToArray() ?? [];
        if (portSnapshot.Any(port => port is null))
        {
            throw new ArgumentException("Port collection must not contain null values.", nameof(ports));
        }

        if (parameterSnapshot.Any(parameter => parameter is null))
        {
            throw new ArgumentException("Parameter collection must not contain null values.", nameof(parameters));
        }

        if (portSnapshot.GroupBy(port => (port.Direction, port.Key)).Any(group => group.Count() > 1))
        {
            throw new ArgumentException("Port keys must be unique within each direction.", nameof(ports));
        }

        if (parameterSnapshot.GroupBy(parameter => parameter.Key).Any(group => group.Count() > 1))
        {
            throw new ArgumentException("Parameter keys must be unique within a node type.", nameof(parameters));
        }

        Id = id ?? throw new ArgumentNullException(nameof(id));
        Version = version ?? throw new ArgumentNullException(nameof(version));
        DisplayName = displayName.Trim();
        Category = category.Trim();
        this.ports = Array.AsReadOnly(portSnapshot);
        this.parameters = Array.AsReadOnly(parameterSnapshot);
        ExecutionCharacteristics = executionCharacteristics ?? new(NodeExecutionLifetime.Stateless);
        Origin = origin ?? NodeTypeOrigin.BuiltIn;
        ExecutorFactory = executorFactory;
    }

    public NodeTypeId Id { get; }

    public SemanticVersion Version { get; }

    public string DisplayName { get; }

    public string Category { get; }

    public IReadOnlyList<PortDefinition> Ports => ports;

    public IReadOnlyList<PortDefinition> InputPorts =>
        ports.Where(port => port.Direction == PortDirection.Input).ToArray();

    public IReadOnlyList<PortDefinition> OutputPorts =>
        ports.Where(port => port.Direction == PortDirection.Output).ToArray();

    public IReadOnlyList<ParameterDefinition> Parameters => parameters;

    public NodeExecutionCharacteristics ExecutionCharacteristics { get; }

    public NodeTypeOrigin Origin { get; }

    public INodeExecutorFactory? ExecutorFactory { get; }

    public PortDefinition? FindPort(PortKey key, PortDirection direction) =>
        ports.FirstOrDefault(port => port.Key == key && port.Direction == direction);

    public ParameterDefinition? FindParameter(ParameterKey key) =>
        parameters.FirstOrDefault(parameter => parameter.Key == key);
}
