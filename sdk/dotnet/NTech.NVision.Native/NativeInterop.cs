using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace NTech.NVision.Native;

internal enum NativeStatus
{
    Ok = 0,
    InvalidArgument = 1,
    AbiMismatch = 2,
    BufferTooSmall = 3,
    InternalError = 100,
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

internal static partial class NativeMethods
{
    private const string LibraryName = "nvision_engine";

    [LibraryImport(LibraryName, EntryPoint = "nv_get_abi_version")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial uint GetAbiVersion();

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_create")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatus EngineCreate(
        in NativeEngineConfig config,
        out IntPtr handle);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_release")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial void EngineRelease(IntPtr handle);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_get_info")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatus EngineGetInfo(
        SafeNativeEngineHandle handle,
        ref NativeEngineInfoInterop info);

    [LibraryImport(LibraryName, EntryPoint = "nv_engine_get_capabilities_json")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatus EngineGetCapabilitiesJson(
        SafeNativeEngineHandle handle,
        IntPtr buffer,
        UIntPtr bufferCapacity,
        out UIntPtr requiredSize);

    [LibraryImport(LibraryName, EntryPoint = "nv_get_last_error_message")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    internal static partial NativeStatus GetLastErrorMessage(
        IntPtr buffer,
        UIntPtr bufferCapacity,
        out UIntPtr requiredSize);
}

internal static class NativeException
{
    public static void ThrowIfFailed(NativeStatus status)
    {
        if (status == NativeStatus.Ok)
        {
            return;
        }

        throw new NativeEngineException(status, ReadLastError());
    }

    private static string ReadLastError()
    {
        NativeStatus sizing = NativeMethods.GetLastErrorMessage(
            IntPtr.Zero,
            UIntPtr.Zero,
            out UIntPtr requiredSize);
        if (sizing != NativeStatus.BufferTooSmall || requiredSize == UIntPtr.Zero)
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
            NativeStatus status = NativeMethods.GetLastErrorMessage(
                buffer,
                requiredSize,
                out UIntPtr writtenSize);
            return status == NativeStatus.Ok && writtenSize == requiredSize
                ? Marshal.PtrToStringUTF8(buffer) ?? "Native diagnostic was invalid UTF-8."
                : "Native diagnostic retrieval failed.";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }
}
