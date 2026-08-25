using System.Runtime.InteropServices;

namespace NTech.NVision.Native;

public sealed class NativeFrameLease : IDisposable
{
    private const ulong MaximumManagedCopyBytes = 256UL * 1024UL * 1024UL;
    private const ulong MaximumMetadataBytes = 1024UL * 1024UL;

    private readonly SafeNativeFrameHandle handle;
    private int disposed;

    private NativeFrameLease(SafeNativeFrameHandle handle) => this.handle = handle;

    internal static NativeFrameLease FromOwned(IntPtr handle)
    {
        SafeNativeFrameHandle safeHandle;
        try
        {
            safeHandle = SafeNativeFrameHandle.FromOwned(handle);
        }
        catch
        {
            NativeMethods.FrameRelease(handle);
            throw;
        }

        try
        {
            return new NativeFrameLease(safeHandle);
        }
        catch
        {
            safeHandle.Dispose();
            throw;
        }
    }

    public unsafe NativeFrameInfo GetInfo()
    {
        ThrowIfDisposed();
        var info = new NativeFrameInfoInterop
        {
            StructSize = (uint)sizeof(NativeFrameInfoInterop),
        };
        NativeException.ThrowIfFailed(NativeMethods.FrameGetInfo(handle, ref info));

        Span<byte> id = stackalloc byte[16];
        for (int index = 0; index < id.Length; index++)
        {
            id[index] = info.Id[index];
        }

        return new NativeFrameInfo(
            new Guid(id),
            ReadBoundedUtf8(metadata: false),
            info.SequenceNumber,
            info.CapturedAtUnixNanoseconds,
            info.ReceivedAtUnixNanoseconds,
            info.Width,
            info.Height,
            info.StrideBytes,
            (NativePixelFormat)info.PixelFormat,
            (NativeFrameStatus)info.Status,
            (NativeTimestampQuality)info.TimestampQuality,
            info.BufferLength);
    }

    public string GetMetadataJson()
    {
        ThrowIfDisposed();
        return ReadBoundedUtf8(metadata: true);
    }

    public byte[] CopyPixels()
    {
        ThrowIfDisposed();
        NativeException.ThrowIfFailed(NativeMethods.FrameGetBuffer(
            handle,
            out IntPtr data,
            out UIntPtr length));
        ulong byteCount = length.ToUInt64();
        if (byteCount > MaximumManagedCopyBytes || byteCount > int.MaxValue)
        {
            throw new InvalidOperationException(
                $"Native frame buffer size '{byteCount}' exceeds the managed diagnostic copy limit.");
        }
        if (byteCount != 0 && data == IntPtr.Zero)
        {
            throw new InvalidOperationException("Native frame returned a null buffer pointer.");
        }

        var pixels = new byte[(int)byteCount];
        if (pixels.Length != 0)
        {
            Marshal.Copy(data, pixels, 0, pixels.Length);
        }
        return pixels;
    }

    public NativeFrameLease Retain()
    {
        ThrowIfDisposed();
        bool addedReference = false;
        try
        {
            handle.DangerousAddRef(ref addedReference);
            IntPtr nativeHandle = handle.DangerousGetHandle();
            NativeException.ThrowIfFailed(NativeMethods.FrameRetain(nativeHandle));
            return FromOwned(nativeHandle);
        }
        finally
        {
            if (addedReference)
            {
                handle.DangerousRelease();
            }
        }
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) == 0)
        {
            handle.Dispose();
        }
    }

    private string ReadBoundedUtf8(bool metadata)
    {
        UIntPtr required;
        NativeStatusCode sizing = metadata
            ? NativeMethods.FrameGetMetadataJson(handle, IntPtr.Zero, UIntPtr.Zero, out required)
            : NativeMethods.FrameGetCameraId(handle, IntPtr.Zero, UIntPtr.Zero, out required);
        if (sizing != NativeStatusCode.BufferTooSmall)
        {
            NativeException.ThrowIfFailed(sizing);
        }

        ulong requestedBytes = required.ToUInt64();
        if (requestedBytes is 0 or > MaximumMetadataBytes)
        {
            throw new InvalidOperationException(
                $"Native frame returned invalid UTF-8 payload size '{requestedBytes}'.");
        }

        IntPtr buffer = Marshal.AllocHGlobal(checked((int)requestedBytes));
        try
        {
            UIntPtr written;
            NativeStatusCode status = metadata
                ? NativeMethods.FrameGetMetadataJson(handle, buffer, required, out written)
                : NativeMethods.FrameGetCameraId(handle, buffer, required, out written);
            NativeException.ThrowIfFailed(status);
            if (written != required)
            {
                throw new InvalidOperationException("Native frame UTF-8 payload size changed during retrieval.");
            }

            return Marshal.PtrToStringUTF8(buffer) ??
                throw new InvalidOperationException("Native frame payload was not valid UTF-8.");
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    private void ThrowIfDisposed() =>
        ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
}
