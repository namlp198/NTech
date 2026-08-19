namespace NTech.NVision.Contracts.Imaging;

public interface IFrameBuffer : IDisposable
{
    public FrameBufferMemoryType MemoryType { get; }

    public int Length { get; }

    public bool IsReadOnly { get; }

    public ReadOnlyMemory<byte> Memory { get; }
}
