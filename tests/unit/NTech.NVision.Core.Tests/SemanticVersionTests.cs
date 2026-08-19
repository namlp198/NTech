using NTech.NVision.Core.Versioning;

namespace NTech.NVision.Core.Tests;

public sealed class SemanticVersionTests
{
    [Theory]
    [InlineData("0.0.0")]
    [InlineData("1.2.3")]
    [InlineData("2.0.0-preview.1")]
    [InlineData("2.0.0-preview.1+build.007")]
    public void ParseRoundTripsSemanticVersion(string value)
    {
        SemanticVersion version = SemanticVersion.Parse(value);

        Assert.Equal(value, version.ToString());
    }

    [Theory]
    [InlineData("1.0")]
    [InlineData("01.0.0")]
    [InlineData("1.0.0-")]
    [InlineData("1.0.0-preview..1")]
    public void ParseRejectsInvalidSemanticVersion(string value)
    {
        Assert.Throws<FormatException>(() => SemanticVersion.Parse(value));
    }

    [Fact]
    public void ReleaseSortsAfterPreRelease()
    {
        SemanticVersion preview = SemanticVersion.Parse("1.0.0-preview.2");
        SemanticVersion release = SemanticVersion.Parse("1.0.0");

        Assert.True(preview < release);
        Assert.True(release > preview);
    }
}
