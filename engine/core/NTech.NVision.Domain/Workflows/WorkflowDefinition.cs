using System.Collections.ObjectModel;
using NTech.NVision.Core.Results;
using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed class WorkflowDefinition
{
    private readonly List<WorkflowConnection> connections = [];
    private readonly ReadOnlyCollection<WorkflowConnection> connectionView;
    private readonly List<WorkflowNode> nodes = [];
    private readonly ReadOnlyCollection<WorkflowNode> nodeView;

    public WorkflowDefinition(
        WorkflowId id,
        string name,
        string schemaVersion = "1.0",
        TimeSpan? timeout = null)
    {
        if (timeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(nameof(timeout), "Workflow timeout must be positive when configured.");
        }

        Id = id ?? throw new ArgumentNullException(nameof(id));
        Name = DomainText.Require(name, nameof(name));
        SchemaVersion = DomainText.Require(schemaVersion, nameof(schemaVersion), 64);
        Timeout = timeout;
        nodeView = nodes.AsReadOnly();
        connectionView = connections.AsReadOnly();
    }

    public WorkflowId Id { get; }

    public string Name { get; }

    public string SchemaVersion { get; }

    public TimeSpan? Timeout { get; }

    public IReadOnlyList<WorkflowNode> Nodes => nodeView;

    public IReadOnlyList<WorkflowConnection> Connections => connectionView;

    public Result<WorkflowNode> AddNode(WorkflowNode node)
    {
        ArgumentNullException.ThrowIfNull(node);

        if (nodes.Any(existing => existing.Id == node.Id))
        {
            return Result.Failure<WorkflowNode>(WorkflowErrors.DuplicateNode(node.Id));
        }

        nodes.Add(node);
        return Result.Success(node);
    }

    public Result<WorkflowConnection> AddConnection(WorkflowConnection connection)
    {
        ArgumentNullException.ThrowIfNull(connection);

        Result<ConnectionEndpoints> endpointResult = ResolveEndpoints(connection);
        if (endpointResult.IsFailure)
        {
            return Result.Failure<WorkflowConnection>(endpointResult.Error!);
        }

        ConnectionEndpoints endpoints = endpointResult.Value;
        if (connections.Any(existing => existing.Id == connection.Id))
        {
            return Result.Failure<WorkflowConnection>(WorkflowErrors.DuplicateConnection(connection.Id));
        }

        if (connection.SourceNodeId == connection.TargetNodeId)
        {
            return Result.Failure<WorkflowConnection>(WorkflowErrors.SelfConnectionNotAllowed(connection.SourceNodeId));
        }

        PortDefinition? sourcePort = endpoints.Source.FindPort(connection.SourcePort, PortDirection.Output);
        if (sourcePort is null)
        {
            return Result.Failure<WorkflowConnection>(
                WorkflowErrors.MissingSourcePort(connection.SourceNodeId, connection.SourcePort));
        }

        PortDefinition? targetPort = endpoints.Target.FindPort(connection.TargetPort, PortDirection.Input);
        if (targetPort is null)
        {
            return Result.Failure<WorkflowConnection>(
                WorkflowErrors.MissingTargetPort(connection.TargetNodeId, connection.TargetPort));
        }

        if (sourcePort.DataType != targetPort.DataType)
        {
            return Result.Failure<WorkflowConnection>(
                WorkflowErrors.IncompatiblePortTypes(sourcePort.DataType, targetPort.DataType));
        }

        if (connections.Any(existing => HasSameRoute(existing, connection)))
        {
            return Result.Failure<WorkflowConnection>(WorkflowErrors.DuplicateRoute());
        }

        if (!sourcePort.MultiConnectionAllowed &&
            connections.Any(existing =>
                existing.SourceNodeId == connection.SourceNodeId && existing.SourcePort == connection.SourcePort))
        {
            return Result.Failure<WorkflowConnection>(
                WorkflowErrors.PortMultiplicityExceeded(connection.SourceNodeId, connection.SourcePort));
        }

        if (!targetPort.MultiConnectionAllowed &&
            connections.Any(existing =>
                existing.TargetNodeId == connection.TargetNodeId && existing.TargetPort == connection.TargetPort))
        {
            return Result.Failure<WorkflowConnection>(
                WorkflowErrors.PortMultiplicityExceeded(connection.TargetNodeId, connection.TargetPort));
        }

        if (PathExists(connection.TargetNodeId, connection.SourceNodeId))
        {
            return Result.Failure<WorkflowConnection>(WorkflowErrors.IllegalCycle());
        }

        connections.Add(connection);
        return Result.Success(connection);
    }

    private static bool HasSameRoute(WorkflowConnection existing, WorkflowConnection candidate) =>
        existing.SourceNodeId == candidate.SourceNodeId &&
        existing.SourcePort == candidate.SourcePort &&
        existing.TargetNodeId == candidate.TargetNodeId &&
        existing.TargetPort == candidate.TargetPort;

    private Result<ConnectionEndpoints> ResolveEndpoints(WorkflowConnection connection)
    {
        WorkflowNode? source = nodes.FirstOrDefault(node => node.Id == connection.SourceNodeId);
        if (source is null)
        {
            return Result.Failure<ConnectionEndpoints>(WorkflowErrors.MissingSourceNode(connection.SourceNodeId));
        }

        WorkflowNode? target = nodes.FirstOrDefault(node => node.Id == connection.TargetNodeId);
        if (target is null)
        {
            return Result.Failure<ConnectionEndpoints>(WorkflowErrors.MissingTargetNode(connection.TargetNodeId));
        }

        return Result.Success(new ConnectionEndpoints(source, target));
    }

    private bool PathExists(NodeId start, NodeId destination)
    {
        var visited = new HashSet<NodeId>();
        var pending = new Stack<NodeId>();
        pending.Push(start);

        while (pending.TryPop(out NodeId? current))
        {
            if (current == destination)
            {
                return true;
            }

            if (!visited.Add(current))
            {
                continue;
            }

            foreach (WorkflowConnection connection in connections.Where(item => item.SourceNodeId == current))
            {
                pending.Push(connection.TargetNodeId);
            }
        }

        return false;
    }

    private sealed record ConnectionEndpoints(WorkflowNode Source, WorkflowNode Target);
}
