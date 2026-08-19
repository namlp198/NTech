namespace NTech.NVision.Domain.Workflows;

public sealed record WorkflowConnection
{
    public WorkflowConnection(
        ConnectionId id,
        NodeId sourceNodeId,
        PortKey sourcePort,
        NodeId targetNodeId,
        PortKey targetPort)
    {
        Id = id ?? throw new ArgumentNullException(nameof(id));
        SourceNodeId = sourceNodeId ?? throw new ArgumentNullException(nameof(sourceNodeId));
        SourcePort = sourcePort ?? throw new ArgumentNullException(nameof(sourcePort));
        TargetNodeId = targetNodeId ?? throw new ArgumentNullException(nameof(targetNodeId));
        TargetPort = targetPort ?? throw new ArgumentNullException(nameof(targetPort));
    }

    public ConnectionId Id { get; }

    public NodeId SourceNodeId { get; }

    public PortKey SourcePort { get; }

    public NodeId TargetNodeId { get; }

    public PortKey TargetPort { get; }
}
