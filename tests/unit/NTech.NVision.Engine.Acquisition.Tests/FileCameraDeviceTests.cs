using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Plugin.Abstractions.Cameras;
using NTech.NVision.Plugin.Camera.File;

namespace NTech.NVision.Engine.Acquisition.Tests;

public sealed class FileCameraDeviceTests
{
    [Fact]
    public async Task ReplaysFixtureSequenceAndLoopsDeterministically()
    {
        string type01 = Fixture("type01.pgm");
        string type04 = Fixture("type04.pgm");
        var camera = new FileCameraDevice(
            new CameraId("wire-marker-file"),
            [type01, type04],
            loop: true,
            new FixedTimeProvider());

        Assert.True((await camera.ConnectAsync()).IsSuccess);
        using FrameLease first = (await camera.GrabAsync(Request())).Value;
        using FrameLease second = (await camera.GrabAsync(Request())).Value;
        using FrameLease looped = (await camera.GrabAsync(Request())).Value;

        AssertFrame(first.Frame, 1, "type01.pgm");
        AssertFrame(second.Frame, 2, "type04.pgm");
        AssertFrame(looped.Frame, 3, "type01.pgm");
        Assert.NotEqual(first.Frame.Buffer.Memory.ToArray(), second.Frame.Buffer.Memory.ToArray());

        FrameId originalId = first.Frame.Id;
        Assert.True((await camera.ConnectAsync()).IsSuccess);
        using FrameLease replayed = (await camera.GrabAsync(Request())).Value;
        Assert.Equal(originalId, replayed.Frame.Id);

        await camera.DisposeAsync();
        await camera.DisposeAsync();
    }

    [Fact]
    public async Task NonLoopingCameraReportsEndOfSequence()
    {
        await using var camera = new FileCameraDevice(
            new CameraId("single-file"),
            [Fixture("type01.pgm")]);
        Assert.True((await camera.ConnectAsync()).IsSuccess);
        using FrameLease lease = (await camera.GrabAsync(Request())).Value;

        var exhausted = await camera.GrabAsync(Request());

        Assert.True(exhausted.IsFailure);
        Assert.Equal("FILECAM-003", exhausted.Error?.Code);
    }

    private static FrameGrabRequest Request() => new(TimeSpan.FromSeconds(1), "wire-marker");

    private static string Fixture(string name) => Path.Combine(AppContext.BaseDirectory, "Fixtures", name);

    private static void AssertFrame(Frame frame, long sequence, string sourceFile)
    {
        Assert.Equal(sequence, frame.SequenceNumber);
        Assert.Equal(8, frame.Width);
        Assert.Equal(8, frame.Height);
        Assert.Equal(8, frame.StrideBytes);
        Assert.Equal(PixelFormat.Mono8, frame.PixelFormat);
        Assert.Equal(FrameStatus.Complete, frame.Status);
        Assert.Equal(DateTimeOffset.UnixEpoch, frame.CapturedAt);
        Assert.Equal(sourceFile, frame.Metadata["sourceFile"]);
        Assert.Equal("wire-marker", frame.Metadata["frameRole"]);
    }

    private sealed class FixedTimeProvider : TimeProvider
    {
        public override DateTimeOffset GetUtcNow() => DateTimeOffset.UnixEpoch;
    }
}
