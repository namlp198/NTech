using System.Runtime.InteropServices;

namespace NTech.NVision.Native;

public sealed class NativeEngine : IDisposable
{
    public const uint SupportedAbiVersion = 1;

    private readonly SafeNativeEngineHandle handle;
    private int disposed;

    private NativeEngine(SafeNativeEngineHandle handle) => this.handle = handle;

    public static NativeEngine Create()
    {
        uint nativeAbiVersion = NativeMethods.GetAbiVersion();
        if (nativeAbiVersion != SupportedAbiVersion)
        {
            throw new InvalidOperationException(
                $"Native engine ABI '{nativeAbiVersion}' is incompatible with managed ABI '{SupportedAbiVersion}'.");
        }

        var config = new NativeEngineConfig
        {
            StructSize = (uint)Marshal.SizeOf<NativeEngineConfig>(),
            RequestedAbiVersion = SupportedAbiVersion,
        };
        NativeStatus status = NativeMethods.EngineCreate(in config, out IntPtr nativeHandle);
        NativeException.ThrowIfFailed(status);
        if (nativeHandle == IntPtr.Zero)
        {
            throw new InvalidOperationException("Native engine returned a null handle after successful creation.");
        }

        return new NativeEngine(SafeNativeEngineHandle.FromOwned(nativeHandle));
    }

    public NativeEngineInfo GetInfo()
    {
        ThrowIfDisposed();
        var info = new NativeEngineInfoInterop
        {
            StructSize = (uint)Marshal.SizeOf<NativeEngineInfoInterop>(),
        };
        NativeException.ThrowIfFailed(NativeMethods.EngineGetInfo(handle, ref info));
        return new NativeEngineInfo(
            info.AbiVersion,
            new Version((int)info.VersionMajor, (int)info.VersionMinor, (int)info.VersionPatch),
            info.FeatureFlags);
    }

    public string GetCapabilitiesJson()
    {
        ThrowIfDisposed();
        NativeStatus sizing = NativeMethods.EngineGetCapabilitiesJson(
            handle,
            IntPtr.Zero,
            UIntPtr.Zero,
            out UIntPtr requiredSize);
        if (sizing != NativeStatus.BufferTooSmall)
        {
            NativeException.ThrowIfFailed(sizing);
        }

        ulong requestedBytes = requiredSize.ToUInt64();
        if (requestedBytes is 0 or > 1024 * 1024)
        {
            throw new InvalidOperationException(
                $"Native engine returned invalid capabilities size '{requestedBytes}'.");
        }

        IntPtr buffer = Marshal.AllocHGlobal(checked((int)requestedBytes));
        try
        {
            NativeException.ThrowIfFailed(NativeMethods.EngineGetCapabilitiesJson(
                handle,
                buffer,
                requiredSize,
                out UIntPtr writtenSize));
            if (writtenSize != requiredSize)
            {
                throw new InvalidOperationException("Native capabilities size changed during retrieval.");
            }

            return Marshal.PtrToStringUTF8(buffer) ??
                throw new InvalidOperationException("Native capabilities payload was not valid UTF-8.");
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) == 0)
        {
            handle.Dispose();
        }
    }

    private void ThrowIfDisposed() =>
        ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
}
