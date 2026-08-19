using System.Collections.ObjectModel;
using NTech.NVision.Core.Results;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Registry;

public sealed class NodeTypeRegistry : INodeTypeRegistry
{
    private readonly Dictionary<NodeTypeKey, NodeTypeDefinition> definitions = [];
    private readonly List<NodeTypeDefinition> definitionList = [];
    private readonly ReadOnlyCollection<NodeTypeDefinition> definitionView;

    public NodeTypeRegistry() => definitionView = definitionList.AsReadOnly();

    public IReadOnlyCollection<NodeTypeDefinition> Definitions => definitionView;

    public Result<NodeTypeDefinition> Register(NodeTypeDefinition definition)
    {
        ArgumentNullException.ThrowIfNull(definition);
        var key = new NodeTypeKey(definition.Id, definition.Version);
        if (definitions.ContainsKey(key))
        {
            return Result.Failure<NodeTypeDefinition>(
                NodeTypeRegistryErrors.Duplicate(definition.Id, definition.Version));
        }

        definitions.Add(key, definition);
        definitionList.Add(definition);
        return Result.Success(definition);
    }

    public Result<NodeTypeDefinition> Resolve(NodeTypeId nodeTypeId, SemanticVersion version)
    {
        ArgumentNullException.ThrowIfNull(nodeTypeId);
        ArgumentNullException.ThrowIfNull(version);

        if (definitions.TryGetValue(new NodeTypeKey(nodeTypeId, version), out NodeTypeDefinition? definition))
        {
            return Result.Success(definition);
        }

        return definitions.Keys.Any(key => key.Id == nodeTypeId)
            ? Result.Failure<NodeTypeDefinition>(NodeTypeRegistryErrors.UnsupportedVersion(nodeTypeId, version))
            : Result.Failure<NodeTypeDefinition>(NodeTypeRegistryErrors.Unknown(nodeTypeId));
    }

    private sealed record NodeTypeKey(NodeTypeId Id, SemanticVersion Version);
}
