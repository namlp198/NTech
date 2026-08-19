using System.Diagnostics;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;
using NTech.NVision.Engine.Acquisition.Services;
using NTech.NVision.Plugin.Abstractions.Cameras;

namespace NTech.NVision.Engine.Acquisition.Tests;

public sealed class AcquisitionServiceTests
{
    [Fact]
    public async Task UnknownCameraReturnsExplicitFailure()
    {
        await using var service = new AcquisitionService([]);

        Result<FrameLease> result = await service.AcquireAsync(
            new CameraId("missing"),
            Request());

        Assert.True(result.IsFailure);
        Assert.Equal("ACQ-001", result.Error?.Code);
    }

    [Fact]
    public async Task TimeoutReturnsWhenCameraIgnoresCancellation()
    {
        var camera = new StubCamera(
            new CameraId("blocked"),
            new TaskCompletionSource<Result<FrameLease>>(TaskCreationOptions.RunContinuationsAsynchronously));
        await using var service = new AcquisitionService([camera]);
        var stopwatch = Stopwatch.StartNew();

        Result<FrameLease> result = await service.AcquireAsync(camera.Id, Request(TimeSpan.FromMilliseconds(50)));

        stopwatch.Stop();
        Assert.True(result.IsFailure);
        Assert.Equal("ACQ-002", result.Error?.Code);
        Assert.True(stopwatch.Elapsed < TimeSpan.FromSeconds(2));
    }

    [Fact]
    public async Task LateSuccessfulGrabIsReleasedAfterTimeout()
    {
        var completion = new TaskCompletionSource<Result<FrameLease>>(TaskCreationOptions.RunContinuationsAsynchronously);
        var camera = new StubCamera(new CameraId("late"), completion);
        var buffer = new CountingFrameBuffer();
        await using var service = new AcquisitionService([camera]);

        Result<FrameLease> result = await service.AcquireAsync(camera.Id, Request(TimeSpan.FromMilliseconds(50)));
        completion.SetResult(Result.Success(new FrameLease(CreateFrame(camera.Id, buffer))));
        await WaitUntilAsync(() => buffer.DisposeCount == 1);

        Assert.Equal("ACQ-002", result.Error?.Code);
        Assert.Equal(1, buffer.DisposeCount);
    }

    [Theory]
    [InlineData(false, FrameStatus.Complete, "ACQ-006")]
    [InlineData(true, FrameStatus.Incomplete, "ACQ-004")]
    public async Task InvalidCameraFrameIsRejectedAndReleased(
        bool correctCamera,
        FrameStatus status,
        string expectedCode)
    {
        var buffer = new CountingFrameBuffer();
        var requestedId = new CameraId("requested");
        CameraId returnedId = correctCamera ? requestedId : new CameraId("different");
        var completion = new TaskCompletionSource<Result<FrameLease>>(TaskCreationOptions.RunContinuationsAsynchronously);
        completion.SetResult(Result.Success(new FrameLease(CreateFrame(returnedId, buffer, status))));
        var camera = new StubCamera(requestedId, completion);
        await using var service = new AcquisitionService([camera]);

        Result<FrameLease> result = await service.AcquireAsync(requestedId, Request());

        Assert.Equal(expectedCode, result.Error?.Code);
        Assert.Equal(1, buffer.DisposeCount);
    }

    private static FrameGrabRequest Request(TimeSpan? timeout = null) =>
        new(timeout ?? TimeSpan.FromSeconds(1), "wire-marker");

    private static Frame CreateFrame(
        CameraId cameraId,
        IFrameBuffer buffer,
        FrameStatus status = FrameStatus.Complete) =>
        new(
            FrameId.New(),
            cameraId,
            1,
            DateTimeOffset.UnixEpoch,
            DateTimeOffset.UnixEpoch,
            2,
            2,
            2,
            PixelFormat.Mono8,
            buffer,
            status);

    private static async Task WaitUntilAsync(Func<bool> condition)
    {
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(2));
        while (!condition())
        {
            await Task.Delay(TimeSpan.FromMilliseconds(10), timeout.Token);
        }
    }

    private sealed class StubCamera(
        CameraId id,
        TaskCompletionSource<Result<FrameLease>> completion) : ICameraDevice
    {
        public CameraId Id { get; } = id;

        public CameraDeviceState State { get; private set; } = CameraDeviceState.Disconnected;

        public ValueTask<Result<CameraDeviceState>> ConnectAsync(CancellationToken cancellationToken = default)
        {
            State = CameraDeviceState.Connected;
            return ValueTask.FromResult(Result.Success(State));
        }

        public ValueTask<Result<FrameLease>> GrabAsync(
            FrameGrabRequest request,
            CancellationToken cancellationToken = default) =>
            new(completion.Task);

        public ValueTask<Result<CameraDeviceState>> DisconnectAsync(CancellationToken cancellationToken = default)
        {
            State = CameraDeviceState.Disconnected;
            return ValueTask.FromResult(Result.Success(State));
        }

        public ValueTask DisposeAsync()
        {
            State = CameraDeviceState.Disposed;
            return ValueTask.CompletedTask;
        }
    }

    private sealed class CountingFrameBuffer : IFrameBuffer
    {
        private readonly byte[] bytes = new byte[4];

        public int DisposeCount { get; private set; }

        public FrameBufferMemoryType MemoryType => FrameBufferMemoryType.Managed;

        public int Length => bytes.Length;

        public bool IsReadOnly => true;

        public ReadOnlyMemory<byte> Memory => bytes;

        public void Dispose() => DisposeCount++;
    }
}
