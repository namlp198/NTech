using NTech.NVision.Contracts.Imaging;

namespace NTech.NVision.Engine.Acquisition.Imaging;

public sealed class ManagedFrameBuffer : IFrameBuffer
{
    private byte[]? bytes;

    private ManagedFrameBuffer(byte[] bytes) => this.bytes = bytes;

    public FrameBufferMemoryType MemoryType => FrameBufferMemoryType.Managed;

    public int Length => GetBytes().Length;

    public bool IsReadOnly => true;

    public ReadOnlyMemory<byte> Memory => GetBytes();

    public static ManagedFrameBuffer CopyFrom(ReadOnlySpan<byte> source) =>
        new(source.ToArray());

    public static ManagedFrameBuffer TakeOwnership(byte[] source) =>
        new(source ?? throw new ArgumentNullException(nameof(source)));

    public void Dispose() => Interlocked.Exchange(ref bytes, null);

    private byte[] GetBytes() => bytes ?? throw new ObjectDisposedException(nameof(ManagedFrameBuffer));
}
