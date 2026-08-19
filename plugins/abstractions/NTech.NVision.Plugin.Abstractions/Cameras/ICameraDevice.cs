using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Core.Results;

namespace NTech.NVision.Plugin.Abstractions.Cameras;

public interface ICameraDevice : IAsyncDisposable
{
    public CameraId Id { get; }

    public CameraDeviceState State { get; }

    public ValueTask<Result<CameraDeviceState>> ConnectAsync(CancellationToken cancellationToken = default);

    public ValueTask<Result<FrameLease>> GrabAsync(
        FrameGrabRequest request,
        CancellationToken cancellationToken = default);

    public ValueTask<Result<CameraDeviceState>> DisconnectAsync(CancellationToken cancellationToken = default);
}
