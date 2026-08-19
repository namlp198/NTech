namespace NTech.NVision.Contracts.Imaging;

public sealed class FrameLease : IDisposable
{
    private readonly SharedFrameLease shared;
    private int disposed;

    public FrameLease(Frame frame) => shared = new SharedFrameLease(
        frame ?? throw new ArgumentNullException(nameof(frame)));

    private FrameLease(SharedFrameLease shared) => this.shared = shared;

    public Frame Frame
    {
        get
        {
            ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
            return shared.Frame;
        }
    }

    public FrameLease Retain()
    {
        ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
        shared.AddReference();
        if (Volatile.Read(ref disposed) == 0)
        {
            return new FrameLease(shared);
        }

        shared.Release();
        throw new ObjectDisposedException(nameof(FrameLease));
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) == 0)
        {
            shared.Release();
        }
    }

    private sealed class SharedFrameLease
    {
        private int referenceCount = 1;

        public SharedFrameLease(Frame frame) => Frame = frame;

        public Frame Frame { get; }

        public void AddReference()
        {
            int observed = Volatile.Read(ref referenceCount);
            while (observed > 0)
            {
                int original = Interlocked.CompareExchange(ref referenceCount, observed + 1, observed);
                if (original == observed)
                {
                    return;
                }

                observed = original;
            }

            throw new ObjectDisposedException(nameof(FrameLease));
        }

        public void Release()
        {
            if (Interlocked.Decrement(ref referenceCount) == 0)
            {
                Frame.Buffer.Dispose();
            }
        }
    }
}
