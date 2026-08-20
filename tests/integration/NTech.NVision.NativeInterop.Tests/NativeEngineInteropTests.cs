using System.Text.Json;
using NTech.NVision.Native;

namespace NTech.NVision.NativeInterop.Tests;

public sealed class NativeEngineInteropTests
{
    [Fact]
    public void ManagedFacadeNegotiatesVersionedNativeAbi()
    {
        using NativeEngine engine = NativeEngine.Create();

        NativeEngineInfo info = engine.GetInfo();

        Assert.Equal(NativeEngine.SupportedAbiVersion, info.AbiVersion);
        Assert.Equal(new Version(0, 1, 0), info.EngineVersion);
        Assert.NotEqual(0UL, info.FeatureFlags);
    }

    [Fact]
    public void CapabilitiesCrossAbiAsBoundedUtf8Payload()
    {
        using NativeEngine engine = NativeEngine.Create();

        using JsonDocument capabilities = JsonDocument.Parse(engine.GetCapabilitiesJson());

        Assert.Equal(1, capabilities.RootElement.GetProperty("abiVersion").GetInt32());
        Assert.Contains(
            capabilities.RootElement.GetProperty("features").EnumerateArray(),
            feature => feature.GetString() == "native-core");
    }

    [Fact]
    public void DisposedFacadeRejectsFurtherCalls()
    {
        NativeEngine engine = NativeEngine.Create();
        engine.Dispose();

        Assert.Throws<ObjectDisposedException>(() => engine.GetInfo());
    }
}
