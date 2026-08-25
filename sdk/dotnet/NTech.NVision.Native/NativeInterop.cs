using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace NTech.NVision.Native;

public enum NativeStatusCode
{
    Ok = 0,
    InvalidArgument = 1,
    AbiMismatch = 2,
    BufferTooSmall = 3,
    Timeout = 4,
    Cancelled = 5,
    InvalidState = 6,
    NotFound = 7,
    EndOfStream = 8,
    InvalidData = 9,
    IoError = 10,
    InternalError = 100,
}

internal enum NativeCameraStateInterop
{
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Faulted = 3,
}

internal enum NativePixelFormatInterop
{
    Mono8 = 0,
    Mono16 = 1,
    Rgb8 = 2,
    Bgr8 = 3,
}

internal enum NativeFrameStatusInterop
{
    Complete = 0,
    Incomplete = 1,
    Corrupt = 2,
    Dropped = 3,
    Timeout = 4,
    Cancelled = 5,
}

internal enum NativeTimestampQualityInterop
{
    HostOnly = 0,
    CameraUnsynchronized = 1,
    CameraSynchronized = 2,
    PtpSynchronized = 3,
    ExternalClock = 4,
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeEngineConfig
{
    public uint StructSize;
    public uint RequestedAbiVersion;
    public ulong Reserved0;
    public ulong Reserved1;
    public ulong Reserved2;
    public ulong Reserved3;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeEngineInfoInterop
{
    public uint StructSize;
    public uint AbiVersion;
    public uint VersionMajor;
    public uint VersionMinor;
    public uint VersionPatch;
    public ulong FeatureFlags;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeUtf8View
{
    public IntPtr Data;
    public UIntPtr Length;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeFileCameraConfig
{
    public uint StructSize;
    public NativeUtf8View CameraId;
    public IntPtr Sources;
    public UIntPtr SourceCount;
    public uint Loop;
    public uint Reserved0;
    public ulong Reserved1;
    public ulong Reserved2;
    public ulong Reserved3;
    public ulong Reserved4;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeAcquireRequest
{
    public uint StructSize;
    public uint Reserved0;
    public ulong TimeoutNanoseconds;
    public NativeUtf8View FrameRole;
    public IntPtr Cancellation;
    public ulong Reserved1;
    public ulong Reserved2;
    public ulong Reserved3;
    public ulong Reserved4;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeFrameInfoInterop
{
    public uint StructSize;
    public fixed byte Id[16];
    public ulong SequenceNumber;
    public long CapturedAtUnixNanoseconds;
    public long ReceivedAtUnixNanoseconds;
    public uint Width;
    public uint Height;
    public uint StrideBytes;
    public NativePixelFormatInterop PixelFormat;
    public NativeFrameStatusInterop Status;
    public NativeTimestampQualityInterop TimestampQuality;
    public ulong BufferLength;
}

internal sealed class SafeNativeEngineHandle : SafeHandle
{
    internal SafeNativeEngineHandle()
        : base(IntPtr.Zero, ownsHandle: true)
    {
    }

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal static SafeNativeEngineHandle FromOwned(IntPtr handle)
    {
        var safeHandle = new SafeNativeEngineHandle();
        safeHandle.SetHandle(handle);
        return safeHandle;
    }

    protected override bool ReleaseHandle()
    {
        NativeMethods.EngineRelease(handle);
        return true;
    }
}

internal sealed class SafeNativeCameraHandle : SafeHandle
{
    internal SafeNativeCameraHandle()
        : base(IntPtr.Zero, ownsHandle: true)
    {
    }

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal static SafeNativeCameraHandle FromOwned(IntPtr handle)
    {
        var safeHandle = new SafeNativeCameraHandle();
        safeHandle.SetHandle(handle);
        return safeHandle;
    }

    protected override bool ReleaseHandle()
    {
        NativeMethods.CameraRelease(handle);
        return true;
    }
}

internal sealed class SafeNativeFrameHandle : SafeHandle
{
    internal SafeNativeFrameHandle()
        : base(IntPtr.Zero, ownsHandle: true)
    {
    }

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal static SafeNativeFrameHandle FromOwned(IntPtr handle)
    {
        var safeHandle = new SafeNativeFrameHandle();
        safeHandle.SetHandle(handle);
        return safeHandle;
    }

    protected override bool ReleaseHandle()
    {
        NativeMethods.FrameRelease(handle);
        return true;
    }
}

internal sealed class SafeNativeCancellationHandle : SafeHandle
{
    internal SafeNativeCancellationHandle()
        : base(IntPtr.Zero, ownsHandle: true)
    {
    }

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal static SafeNativeCancellationHandle Create()
    {
        NativeException.ThrowIfFailed(NativeMethods.CancelSourceCreate(out IntPtr handle));
        if (handle == IntPtr.Zero)
        {
            throw new InvalidOperationException(
                "Native engine returned a null cancellation handle after successful creation.");
        }

        try
        {
            return FromOwned(handle);
        }
        catch
        {
            NativeMethods.CancelSourceRelease(handle);
            throw;
        }
    }

    internal void CancelNoThrow() => _ = NativeMethods.CancelSourceCancel(this);

    private static SafeNativeCancellationHandle FromOwned(IntPtr handle)
    {
        var safeHandle = new SafeNativeCancellationHandle();
        safeHandle.SetHandle(handle);
        return safeHandle;
    }

    protected override bool ReleaseHandle()
    {
        NativeMethods.CancelSourceRelease(handle);
        return true;
    }
}

internal static partial class NativeMethods
{
    private const string LibraryName = "nvision_engine";

    [LibraryImport(LibraryName, EntryPoint = "nv_get_abi_version")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial uint GetAbiVersion();

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_create")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode EngineCreate(
        in NativeEngineConfig config,
        out IntPtr handle);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_release")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial void EngineRelease(IntPtr handle);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_get_info")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode EngineGetInfo(
        SafeNativeEngineHandle handle,
        ref NativeEngineInfoInterop info);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_get_capabilities_json")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode EngineGetCapabilitiesJson(
        SafeNativeEngineHandle handle,
        IntPtr buffer,
        UIntPtr bufferCapacity,
        out UIntPtr requiredSize);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_create_file_camera")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode EngineCreateFileCamera(
        SafeNativeEngineHandle handle,
        in NativeFileCameraConfig config,
        out IntPtr camera);

    [LibraryImport(LibraryName, EntryPoint = "nv_camera_release")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial void CameraRelease(IntPtr camera);

    [LibraryImport(LibraryName, EntryPoint = "nv_camera_get_state")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode CameraGetState(
        SafeNativeCameraHandle camera,
        out NativeCameraStateInterop state);

    [LibraryImport(LibraryName, EntryPoint = "nv_camera_connect")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode CameraConnect(SafeNativeCameraHandle camera);

    [LibraryImport(LibraryName, EntryPoint = "nv_camera_acquire")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode CameraAcquire(
        SafeNativeCameraHandle camera,
        in NativeAcquireRequest request,
        out IntPtr frame);

    [LibraryImport(LibraryName, EntryPoint = "nv_camera_disconnect")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode CameraDisconnect(SafeNativeCameraHandle camera);

    [LibraryImport(LibraryName, EntryPoint = "nv_cancel_source_create")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode CancelSourceCreate(out IntPtr cancellation);

    [LibraryImport(LibraryName, EntryPoint = "nv_cancel_source_cancel")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode CancelSourceCancel(
        SafeNativeCancellationHandle cancellation);

    [LibraryImport(LibraryName, EntryPoint = "nv_cancel_source_release")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial void CancelSourceRelease(IntPtr cancellation);

    [LibraryImport(LibraryName, EntryPoint = "nv_frame_retain")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode FrameRetain(IntPtr frame);

    [LibraryImport(LibraryName, EntryPoint = "nv_frame_release")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial void FrameRelease(IntPtr frame);

    [LibraryImport(LibraryName, EntryPoint = "nv_frame_get_info")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode FrameGetInfo(
        SafeNativeFrameHandle frame,
        ref NativeFrameInfoInterop info);

    [LibraryImport(LibraryName, EntryPoint = "nv_frame_get_camera_id")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode FrameGetCameraId(
        SafeNativeFrameHandle frame,
        IntPtr buffer,
        UIntPtr bufferCapacity,
        out UIntPtr requiredSize);

    [LibraryImport(LibraryName, EntryPoint = "nv_frame_get_metadata_json")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode FrameGetMetadataJson(
        SafeNativeFrameHandle frame,
        IntPtr buffer,
        UIntPtr bufferCapacity,
        out UIntPtr requiredSize);

    [LibraryImport(LibraryName, EntryPoint = "nv_frame_get_buffer")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode FrameGetBuffer(
        SafeNativeFrameHandle frame,
        out IntPtr data,
        out UIntPtr length);

    [LibraryImport(LibraryName, EntryPoint = "nv_get_last_error_message")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatusCode GetLastErrorMessage(
        IntPtr buffer,
        UIntPtr bufferCapacity,
        out UIntPtr requiredSize);
}

internal static class NativeException
{
    public static void ThrowIfFailed(NativeStatusCode status)
    {
        if (status == NativeStatusCode.Ok)
        {
            return;
        }

        throw new NativeEngineException(status, ReadLastError());
    }

    private static string ReadLastError()
    {
        NativeStatusCode sizing = NativeMethods.GetLastErrorMessage(
            IntPtr.Zero,
            UIntPtr.Zero,
            out UIntPtr requiredSize);
        if (sizing != NativeStatusCode.BufferTooSmall || requiredSize == UIntPtr.Zero)
        {
            return "No native diagnostic was available.";
        }

        ulong requestedBytes = requiredSize.ToUInt64();
        if (requestedBytes > 64 * 1024)
        {
            return "Native diagnostic exceeded the maximum supported size.";
        }

        IntPtr buffer = Marshal.AllocHGlobal(checked((int)requestedBytes));
        try
        {
            NativeStatusCode status = NativeMethods.GetLastErrorMessage(
                buffer,
                requiredSize,
                out UIntPtr writtenSize);
            return status == NativeStatusCode.Ok && writtenSize == requiredSize
                ? Marshal.PtrToStringUTF8(buffer) ?? "Native diagnostic was invalid UTF-8."
                : "Native diagnostic retrieval failed.";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }
}
