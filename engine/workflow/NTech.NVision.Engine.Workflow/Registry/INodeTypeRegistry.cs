using NTech.NVision.Core.Results;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Registry;

public interface INodeTypeRegistry
{
    public IReadOnlyCollection<NodeTypeDefinition> Definitions { get; }

    public Result<NodeTypeDefinition> Register(NodeTypeDefinition definition);

    public Result<NodeTypeDefinition> Resolve(NodeTypeId nodeTypeId, SemanticVersion version);
}
