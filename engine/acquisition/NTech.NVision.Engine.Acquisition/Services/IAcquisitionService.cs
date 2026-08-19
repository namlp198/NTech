using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Core.Results;
using NTech.NVision.Plugin.Abstractions.Cameras;

namespace NTech.NVision.Engine.Acquisition.Services;

public interface IAcquisitionService : IAsyncDisposable
{
    public ValueTask<Result<CameraDeviceState>> ConnectAsync(
        CameraId cameraId,
        CancellationToken cancellationToken = default);

    public ValueTask<Result<FrameLease>> AcquireAsync(
        CameraId cameraId,
        FrameGrabRequest request,
        CancellationToken cancellationToken = default);

    public ValueTask<Result<CameraDeviceState>> DisconnectAsync(
        CameraId cameraId,
        CancellationToken cancellationToken = default);
}
