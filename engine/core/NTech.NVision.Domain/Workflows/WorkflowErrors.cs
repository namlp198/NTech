using NTech.NVision.Core.Errors;

namespace NTech.NVision.Domain.Workflows;

public static class WorkflowErrors
{
    public static DomainError DuplicateNode(NodeId nodeId) =>
        new("WFL-001", $"Node '{nodeId}' already exists in the Workflow.");

    public static DomainError MissingSourceNode(NodeId nodeId) =>
        new("WFL-002", $"Source node '{nodeId}' does not exist in the Workflow.");

    public static DomainError MissingTargetNode(NodeId nodeId) =>
        new("WFL-003", $"Target node '{nodeId}' does not exist in the Workflow.");

    public static DomainError IllegalCycle() =>
        new("WFL-004", "The connection would introduce a cycle into the Workflow.");

    public static DomainError IncompatiblePortTypes(PortDataType source, PortDataType target) =>
        new("WFL-005", $"Port data types '{source}' and '{target}' are not compatible.");

    public static DomainError DuplicateConnection(ConnectionId connectionId) =>
        new("WFL-011", $"Connection '{connectionId}' already exists in the Workflow.");

    public static DomainError MissingSourcePort(NodeId nodeId, PortKey port) =>
        new("WFL-012", $"Output port '{port}' does not exist on source node '{nodeId}'.");

    public static DomainError MissingTargetPort(NodeId nodeId, PortKey port) =>
        new("WFL-013", $"Input port '{port}' does not exist on target node '{nodeId}'.");

    public static DomainError PortMultiplicityExceeded(NodeId nodeId, PortKey port) =>
        new("WFL-014", $"Port '{port}' on node '{nodeId}' does not allow another connection.");

    public static DomainError SelfConnectionNotAllowed(NodeId nodeId) =>
        new("WFL-015", $"Node '{nodeId}' cannot connect to itself.");

    public static DomainError DuplicateRoute() =>
        new("WFL-016", "An identical source-to-target port connection already exists.");
}
