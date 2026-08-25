namespace NTech.NVision.Native;

public enum NativePixelFormat
{
    Mono8 = 0,
    Mono16 = 1,
    Rgb8 = 2,
    Bgr8 = 3,
}

public enum NativeFrameStatus
{
    Complete = 0,
    Incomplete = 1,
    Corrupt = 2,
    Dropped = 3,
    Timeout = 4,
    Cancelled = 5,
}

public enum NativeTimestampQuality
{
    HostOnly = 0,
    CameraUnsynchronized = 1,
    CameraSynchronized = 2,
    PtpSynchronized = 3,
    ExternalClock = 4,
}

public sealed record NativeFrameInfo(
    Guid Id,
    string CameraId,
    ulong SequenceNumber,
    long CapturedAtUnixNanoseconds,
    long ReceivedAtUnixNanoseconds,
    uint Width,
    uint Height,
    uint StrideBytes,
    NativePixelFormat PixelFormat,
    NativeFrameStatus Status,
    NativeTimestampQuality TimestampQuality,
    ulong BufferLength);
