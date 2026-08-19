using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Engine.Acquisition.Imaging;

namespace NTech.NVision.Engine.Acquisition.Tests;

public sealed class FrameLeaseTests
{
    [Fact]
    public void RetainedLeasesReleaseBufferOnlyAfterLastOwner()
    {
        var buffer = new CountingFrameBuffer(4);
        var first = new FrameLease(CreateFrame(buffer));
        FrameLease second = first.Retain();
        FrameLease third = first.Retain();

        first.Dispose();
        second.Dispose();
        Assert.Equal(0, buffer.DisposeCount);

        third.Dispose();
        third.Dispose();

        Assert.Equal(1, buffer.DisposeCount);
        Assert.Throws<ObjectDisposedException>(() => _ = first.Frame);
        Assert.Throws<ObjectDisposedException>(() => first.Retain());
    }

    [Fact]
    public void CompleteFrameRejectsBufferSmallerThanDeclaredShape()
    {
        var buffer = new CountingFrameBuffer(3);

        Assert.Throws<ArgumentException>(() => CreateFrame(buffer));
    }

    [Fact]
    public void ManagedBufferCopiesInputAndRejectsAccessAfterDispose()
    {
        byte[] source = [1, 2, 3, 4];
        ManagedFrameBuffer buffer = ManagedFrameBuffer.CopyFrom(source);
        source[0] = 99;

        Assert.Equal((byte)1, buffer.Memory.Span[0]);

        buffer.Dispose();

        Assert.Throws<ObjectDisposedException>(() => _ = buffer.Memory);
        Assert.Throws<ObjectDisposedException>(() => _ = buffer.Length);
    }

    private static Frame CreateFrame(IFrameBuffer buffer) => new(
        FrameId.New(),
        new CameraId("test-camera"),
        1,
        DateTimeOffset.UnixEpoch,
        DateTimeOffset.UnixEpoch,
        2,
        2,
        2,
        PixelFormat.Mono8,
        buffer);

    private sealed class CountingFrameBuffer(int length) : IFrameBuffer
    {
        private readonly byte[] bytes = new byte[length];

        public int DisposeCount { get; private set; }

        public FrameBufferMemoryType MemoryType => FrameBufferMemoryType.Managed;

        public int Length => bytes.Length;

        public bool IsReadOnly => true;

        public ReadOnlyMemory<byte> Memory => bytes;

        public void Dispose() => DisposeCount++;
    }
}
