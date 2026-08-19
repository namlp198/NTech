namespace NTech.NVision.Contracts.Imaging;

public enum FrameTimestampQuality
{
    HostOnly,
    CameraUnsynchronized,
    CameraSynchronized,
    PtpSynchronized,
    ExternalClock,
}
