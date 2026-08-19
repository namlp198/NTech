using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Registry;

public static class NodeTypeRegistryErrors
{
    public static DomainError Duplicate(NodeTypeId id, SemanticVersion version) =>
        new("NTR-001", $"Node type '{id}' version '{version}' is already registered.");

    public static DomainError Unknown(NodeTypeId id) =>
        new("WFL-007", $"Node type '{id}' is not registered.");

    public static DomainError UnsupportedVersion(NodeTypeId id, SemanticVersion version) =>
        new("WFL-008", $"Node type '{id}' version '{version}' is not registered.");
}
