using System.Runtime.InteropServices;

namespace NTech.NVision.Native;

public sealed class NativeFileCamera : IDisposable
{
    private readonly SafeNativeCameraHandle handle;
    private int disposed;

    private NativeFileCamera(SafeNativeCameraHandle handle) => this.handle = handle;

    public NativeCameraState State
    {
        get
        {
            ThrowIfDisposed();
            NativeException.ThrowIfFailed(NativeMethods.CameraGetState(handle, out NativeCameraStateInterop state));
            return (NativeCameraState)state;
        }
    }

    internal static NativeFileCamera Create(
        SafeNativeEngineHandle engine,
        string cameraId,
        IEnumerable<string> sources,
        bool loop)
    {
        if (string.IsNullOrWhiteSpace(cameraId))
        {
            throw new ArgumentException("Camera identifier is required.", nameof(cameraId));
        }

        ArgumentNullException.ThrowIfNull(sources);
        string[] sourcePaths = sources
            .Select(source => string.IsNullOrWhiteSpace(source)
                ? throw new ArgumentException("File Camera source paths are required.", nameof(sources))
                : Path.GetFullPath(source))
            .ToArray();
        if (sourcePaths.Length == 0)
        {
            throw new ArgumentException("File Camera requires at least one source.", nameof(sources));
        }

        using var cameraIdAllocation = new NativeUtf8Allocation(cameraId.Trim());
        var sourceAllocations = new List<NativeUtf8Allocation>(sourcePaths.Length);
        IntPtr sourceViews = IntPtr.Zero;
        try
        {
            foreach (string path in sourcePaths)
            {
                sourceAllocations.Add(new NativeUtf8Allocation(path));
            }

            int viewSize = Marshal.SizeOf<NativeUtf8View>();
            sourceViews = Marshal.AllocHGlobal(checked(viewSize * sourceAllocations.Count));
            for (int index = 0; index < sourceAllocations.Count; index++)
            {
                Marshal.StructureToPtr(
                    sourceAllocations[index].View,
                    IntPtr.Add(sourceViews, checked(index * viewSize)),
                    fDeleteOld: false);
            }

            var config = new NativeFileCameraConfig
            {
                StructSize = (uint)Marshal.SizeOf<NativeFileCameraConfig>(),
                CameraId = cameraIdAllocation.View,
                Sources = sourceViews,
                SourceCount = (UIntPtr)sourceAllocations.Count,
                Loop = loop ? 1U : 0U,
            };
            NativeStatusCode status = NativeMethods.EngineCreateFileCamera(
                engine,
                in config,
                out IntPtr nativeHandle);
            if (status != NativeStatusCode.Ok)
            {
                if (nativeHandle != IntPtr.Zero)
                {
                    NativeMethods.CameraRelease(nativeHandle);
                }
                NativeException.ThrowIfFailed(status);
            }
            if (nativeHandle == IntPtr.Zero)
            {
                throw new InvalidOperationException(
                    "Native engine returned a null File Camera handle after successful creation.");
            }

            SafeNativeCameraHandle safeHandle;
            try
            {
                safeHandle = SafeNativeCameraHandle.FromOwned(nativeHandle);
            }
            catch
            {
                NativeMethods.CameraRelease(nativeHandle);
                throw;
            }

            try
            {
                return new NativeFileCamera(safeHandle);
            }
            catch
            {
                safeHandle.Dispose();
                throw;
            }
        }
        finally
        {
            if (sourceViews != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(sourceViews);
            }
            foreach (NativeUtf8Allocation allocation in sourceAllocations)
            {
                allocation.Dispose();
            }
        }
    }

    public void Connect()
    {
        ThrowIfDisposed();
        NativeException.ThrowIfFailed(NativeMethods.CameraConnect(handle));
    }

    public NativeFrameLease Acquire(
        TimeSpan timeout,
        string frameRole = "source",
        CancellationToken cancellationToken = default)
    {
        ThrowIfDisposed();
        if (timeout <= TimeSpan.Zero || timeout > TimeSpan.FromHours(24))
        {
            throw new ArgumentOutOfRangeException(
                nameof(timeout),
                "Frame acquisition timeout must be positive and no greater than 24 hours.");
        }
        if (string.IsNullOrWhiteSpace(frameRole))
        {
            throw new ArgumentException("Frame role is required.", nameof(frameRole));
        }

        using var role = new NativeUtf8Allocation(frameRole.Trim());
        using SafeNativeCancellationHandle cancellation = SafeNativeCancellationHandle.Create();
        using CancellationTokenRegistration registration = cancellationToken.Register(
            static state => ((SafeNativeCancellationHandle)state!).CancelNoThrow(),
            cancellation);
        var request = new NativeAcquireRequest
        {
            StructSize = (uint)Marshal.SizeOf<NativeAcquireRequest>(),
            TimeoutNanoseconds = checked((ulong)timeout.Ticks * 100UL),
            FrameRole = role.View,
            Cancellation = cancellation.DangerousGetHandle(),
        };
        NativeStatusCode status = NativeMethods.CameraAcquire(
            handle,
            in request,
            out IntPtr nativeFrame);
        if (status != NativeStatusCode.Ok)
        {
            if (nativeFrame != IntPtr.Zero)
            {
                NativeMethods.FrameRelease(nativeFrame);
            }
            NativeException.ThrowIfFailed(status);
        }
        if (nativeFrame == IntPtr.Zero)
        {
            throw new InvalidOperationException(
                "Native engine returned a null frame handle after successful acquisition.");
        }

        return NativeFrameLease.FromOwned(nativeFrame);
    }

    public void Disconnect()
    {
        ThrowIfDisposed();
        NativeException.ThrowIfFailed(NativeMethods.CameraDisconnect(handle));
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
