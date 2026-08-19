using NTech.NVision.Domain.Plugins;
using NTech.NVision.Domain.Projects;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Domain.Tests;

public sealed class StrongIdentifierTests
{
    [Fact]
    public void ProjectIdRoundTripsCanonicalGuid()
    {
        ProjectId identifier = ProjectId.New();

        ProjectId parsed = ProjectId.Parse(identifier.ToString());

        Assert.Equal(identifier, parsed);
    }

    [Fact]
    public void GuidIdentifierRejectsEmptyValue()
    {
        Assert.Throws<ArgumentException>(() => new ProjectId(Guid.Empty));
    }

    [Theory]
    [InlineData("ntech.nodes.crop")]
    [InlineData("ntech.camera.file-v1")]
    [InlineData("customer_1.custom_node")]
    public void NodeTypeIdAcceptsCanonicalStableCode(string value)
    {
        var identifier = new NodeTypeId(value);

        Assert.Equal(value, identifier.Value);
    }

    [Theory]
    [InlineData("")]
    [InlineData("NTech.Nodes.Crop")]
    [InlineData(".ntech.nodes.crop")]
    [InlineData("ntech nodes crop")]
    public void NodeTypeIdRejectsNonCanonicalCode(string value)
    {
        Assert.Throws<ArgumentException>(() => new NodeTypeId(value));
    }

    [Fact]
    public void PluginIdUsesValueEquality()
    {
        var first = new PluginId("ntech.camera.file");
        var second = new PluginId("ntech.camera.file");

        Assert.Equal(first, second);
    }
}
