using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed record PortAddress
{
    public PortAddress(NodeId nodeId, PortKey port)
    {
        NodeId = nodeId ?? throw new ArgumentNullException(nameof(nodeId));
        Port = port ?? throw new ArgumentNullException(nameof(port));
    }

    public NodeId NodeId { get; }

    public PortKey Port { get; }
}
