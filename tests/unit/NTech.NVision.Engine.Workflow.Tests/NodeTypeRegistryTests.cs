using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Plugins;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Workflow.Tests;

public sealed class NodeTypeRegistryTests
{
    [Fact]
    public void RegisterAndResolvePreserveMetadataAndOrigin()
    {
        var registry = new NodeTypeRegistry();
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(
            origin: NodeTypeOrigin.FromPlugin(new PluginId("vendor.vision.nodes")));

        var registration = registry.Register(definition);
        var resolution = registry.Resolve(definition.Id, definition.Version);

        Assert.True(registration.IsSuccess);
        Assert.Same(definition, resolution.Value);
        Assert.Equal("vendor.vision.nodes", resolution.Value.Origin.PluginId?.Value);
    }

    [Fact]
    public void RegisterRejectsDuplicateIdentityAndVersion()
    {
        var registry = new NodeTypeRegistry();
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition();
        registry.Register(definition);

        var result = registry.Register(TestNodeTypes.CreateDefinition());

        Assert.Equal("NTR-001", result.Error?.Code);
    }

    [Fact]
    public void ResolveDistinguishesUnknownTypeFromUnsupportedVersion()
    {
        var registry = new NodeTypeRegistry();
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition();
        registry.Register(definition);

        var unsupported = registry.Resolve(definition.Id, SemanticVersion.Parse("2.0.0"));
        var unknown = registry.Resolve(new NodeTypeId("nvision.test.unknown"), definition.Version);

        Assert.Equal("WFL-008", unsupported.Error?.Code);
        Assert.Equal("WFL-007", unknown.Error?.Code);
    }
}
