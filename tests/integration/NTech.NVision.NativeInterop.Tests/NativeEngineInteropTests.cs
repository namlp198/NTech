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
        Assert.Equal(new Version(0, 2, 0), info.EngineVersion);
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
        Assert.Contains(
            capabilities.RootElement.GetProperty("features").EnumerateArray(),
            feature => feature.GetString() == "native-acquisition");
    }

    [Fact]
    public void DisposedFacadeRejectsFurtherCalls()
    {
        NativeEngine engine = NativeEngine.Create();
        engine.Dispose();

        Assert.Throws<ObjectDisposedException>(() => engine.GetInfo());
    }

    [Fact]
    public void NativeFileCameraOwnsFramesAndLoopsDeterministically()
    {
        NativeEngine engine = NativeEngine.Create();
        using NativeFileCamera camera = engine.CreateFileCamera(
            "wire-marker-file",
            [Fixture("type01.pgm"), Fixture("type04.pgm")],
            loop: true);
        engine.Dispose();

        Assert.Equal(NativeCameraState.Disconnected, camera.State);
        camera.Connect();
        Assert.Equal(NativeCameraState.Connected, camera.State);

        NativeFrameLease first = camera.Acquire(TimeSpan.FromSeconds(1), "wire-marker");
        using NativeFrameLease retained = first.Retain();
        NativeFrameInfo firstInfo = first.GetInfo();
        byte[] firstPixels = first.CopyPixels();
        using JsonDocument metadata = JsonDocument.Parse(first.GetMetadataJson());
        first.Dispose();

        Assert.Equal("wire-marker-file", retained.GetInfo().CameraId);
        Assert.Equal(1UL, firstInfo.SequenceNumber);
        Assert.Equal(8U, firstInfo.Width);
        Assert.Equal(8U, firstInfo.Height);
        Assert.Equal(8U, firstInfo.StrideBytes);
        Assert.Equal(NativePixelFormat.Mono8, firstInfo.PixelFormat);
        Assert.Equal(NativeFrameStatus.Complete, firstInfo.Status);
        Assert.Equal(64UL, firstInfo.BufferLength);
        Assert.Equal((byte)12, firstPixels[0]);
        Assert.Equal("type01.pgm", metadata.RootElement.GetProperty("sourceFile").GetString());
        Assert.Equal("wire-marker", metadata.RootElement.GetProperty("frameRole").GetString());

        using NativeFrameLease second = camera.Acquire(TimeSpan.FromSeconds(1), "wire-marker");
        using NativeFrameLease looped = camera.Acquire(TimeSpan.FromSeconds(1), "wire-marker");
        Assert.Equal(2UL, second.GetInfo().SequenceNumber);
        Assert.Equal(3UL, looped.GetInfo().SequenceNumber);
        Assert.Equal((byte)20, second.CopyPixels()[0]);
        Assert.Equal((byte)12, looped.CopyPixels()[0]);

        Guid originalId = firstInfo.Id;
        camera.Connect();
        using NativeFrameLease replayed = camera.Acquire(TimeSpan.FromSeconds(1), "wire-marker");
        Assert.Equal(originalId, replayed.GetInfo().Id);

        camera.Disconnect();
        Assert.Equal(NativeCameraState.Disconnected, camera.State);
    }

    [Fact]
    public void NativeAcquisitionReportsLifecycleCancellationAndEndOfStream()
    {
        using NativeEngine engine = NativeEngine.Create();
        using NativeFileCamera camera = engine.CreateFileCamera(
            "single-file",
            [Fixture("type01.pgm")]);

        NativeEngineException disconnected = Assert.Throws<NativeEngineException>(
            () => camera.Acquire(TimeSpan.FromSeconds(1)));
        Assert.Equal(NativeStatusCode.InvalidState, disconnected.Status);
        Assert.Contains("FILECAM-001", disconnected.Message, StringComparison.Ordinal);

        camera.Connect();
        using var cancellation = new CancellationTokenSource();
        cancellation.Cancel();
        NativeEngineException cancelled = Assert.Throws<NativeEngineException>(
            () => camera.Acquire(TimeSpan.FromSeconds(1), cancellationToken: cancellation.Token));
        Assert.Equal(NativeStatusCode.Cancelled, cancelled.Status);
        Assert.Contains("ACQ-003", cancelled.Message, StringComparison.Ordinal);

        using NativeFrameLease frame = camera.Acquire(TimeSpan.FromSeconds(1));
        NativeEngineException exhausted = Assert.Throws<NativeEngineException>(
            () => camera.Acquire(TimeSpan.FromSeconds(1)));
        Assert.Equal(NativeStatusCode.EndOfStream, exhausted.Status);
        Assert.Contains("FILECAM-003", exhausted.Message, StringComparison.Ordinal);
    }

    private static string Fixture(string name) =>
        Path.Combine(AppContext.BaseDirectory, "Fixtures", name);
}
