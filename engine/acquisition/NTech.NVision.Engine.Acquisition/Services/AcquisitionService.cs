using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;
using NTech.NVision.Plugin.Abstractions.Cameras;

namespace NTech.NVision.Engine.Acquisition.Services;

public sealed class AcquisitionService : IAcquisitionService
{
    private readonly IReadOnlyDictionary<CameraId, ICameraDevice> cameras;
    private readonly TimeProvider timeProvider;

    public AcquisitionService(
        IEnumerable<ICameraDevice> cameras,
        TimeProvider? timeProvider = null)
    {
        ArgumentNullException.ThrowIfNull(cameras);
        ICameraDevice[] snapshot = cameras.ToArray();
        if (snapshot.Any(camera => camera is null))
        {
            throw new ArgumentException("Camera collection must not contain null values.", nameof(cameras));
        }

        if (snapshot.GroupBy(camera => camera.Id).Any(group => group.Count() > 1))
        {
            throw new ArgumentException("Camera identifiers must be unique.", nameof(cameras));
        }

        this.cameras = snapshot.ToDictionary(camera => camera.Id);
        this.timeProvider = timeProvider ?? TimeProvider.System;
    }

    public async ValueTask<Result<CameraDeviceState>> ConnectAsync(
        CameraId cameraId,
        CancellationToken cancellationToken = default)
    {
        if (!TryGetCamera(cameraId, out ICameraDevice camera, out DomainError? error))
        {
            return Result.Failure<CameraDeviceState>(error!);
        }

        try
        {
            return await camera.ConnectAsync(cancellationToken).ConfigureAwait(false);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            return Result.Failure<CameraDeviceState>(
                new DomainError("ACQ-003", $"Connecting camera '{cameraId}' was cancelled."));
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            return Result.Failure<CameraDeviceState>(
                new DomainError("ACQ-005", $"Camera '{cameraId}' connect failed: {exception.Message}"));
        }
    }

    public async ValueTask<Result<FrameLease>> AcquireAsync(
        CameraId cameraId,
        FrameGrabRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(request);
        if (!TryGetCamera(cameraId, out ICameraDevice camera, out DomainError? error))
        {
            return Result.Failure<FrameLease>(error!);
        }

        using var timeoutSource = new CancellationTokenSource(request.Timeout, timeProvider);
        using var linkedSource = CancellationTokenSource.CreateLinkedTokenSource(
            cancellationToken,
            timeoutSource.Token);
        Task<Result<FrameLease>>? grabTask = null;
        try
        {
            grabTask = camera.GrabAsync(request, linkedSource.Token).AsTask();
            Result<FrameLease> result = await grabTask.WaitAsync(linkedSource.Token).ConfigureAwait(false);
            if (result.IsFailure)
            {
                return result;
            }

            FrameLease lease = result.Value;
            Frame frame = lease.Frame;
            if (frame.CameraId != cameraId)
            {
                lease.Dispose();
                return Result.Failure<FrameLease>(new DomainError(
                    "ACQ-006",
                    $"Camera '{cameraId}' returned a frame owned by '{frame.CameraId}'."));
            }

            if (frame.Status != FrameStatus.Complete)
            {
                lease.Dispose();
                return Result.Failure<FrameLease>(new DomainError(
                    "ACQ-004",
                    $"Camera '{cameraId}' returned frame '{frame.Id}' with status '{frame.Status}'."));
            }

            return result;
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            ObserveAndReleaseAbandonedLease(grabTask);
            return Result.Failure<FrameLease>(
                new DomainError("ACQ-003", $"Frame acquisition from camera '{cameraId}' was cancelled."));
        }
        catch (OperationCanceledException) when (timeoutSource.IsCancellationRequested)
        {
            ObserveAndReleaseAbandonedLease(grabTask);
            return Result.Failure<FrameLease>(new DomainError(
                "ACQ-002",
                $"Frame acquisition from camera '{cameraId}' exceeded timeout '{request.Timeout}'."));
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            return Result.Failure<FrameLease>(
                new DomainError("ACQ-005", $"Camera '{cameraId}' acquisition failed: {exception.Message}"));
        }
    }

    public async ValueTask<Result<CameraDeviceState>> DisconnectAsync(
        CameraId cameraId,
        CancellationToken cancellationToken = default)
    {
        if (!TryGetCamera(cameraId, out ICameraDevice camera, out DomainError? error))
        {
            return Result.Failure<CameraDeviceState>(error!);
        }

        try
        {
            return await camera.DisconnectAsync(cancellationToken).ConfigureAwait(false);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            return Result.Failure<CameraDeviceState>(
                new DomainError("ACQ-003", $"Disconnecting camera '{cameraId}' was cancelled."));
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            return Result.Failure<CameraDeviceState>(
                new DomainError("ACQ-005", $"Camera '{cameraId}' disconnect failed: {exception.Message}"));
        }
    }

    public async ValueTask DisposeAsync()
    {
        List<Exception>? errors = null;
        foreach (ICameraDevice camera in cameras.Values)
        {
            try
            {
                await camera.DisposeAsync().ConfigureAwait(false);
            }
            catch (Exception exception) when (exception is not OutOfMemoryException)
            {
                errors ??= [];
                errors.Add(exception);
            }
        }

        if (errors is not null)
        {
            throw new AggregateException("One or more cameras failed to dispose.", errors);
        }
    }

    private bool TryGetCamera(
        CameraId cameraId,
        out ICameraDevice camera,
        out DomainError? error)
    {
        ArgumentNullException.ThrowIfNull(cameraId);
        if (cameras.TryGetValue(cameraId, out ICameraDevice? registeredCamera))
        {
            camera = registeredCamera;
            error = null;
            return true;
        }

        error = new DomainError("ACQ-001", $"Camera '{cameraId}' is not registered.");
        camera = null!;
        return false;
    }

    private static void ObserveAndReleaseAbandonedLease(Task<Result<FrameLease>>? task)
    {
        if (task is null)
        {
            return;
        }

        if (task.IsCompleted)
        {
            ReleaseCompletedLease(task);
            return;
        }

        _ = task.ContinueWith(
            ReleaseCompletedLease,
            CancellationToken.None,
            TaskContinuationOptions.ExecuteSynchronously,
            TaskScheduler.Default);
    }

    private static void ReleaseCompletedLease(Task<Result<FrameLease>> completed)
    {
        if (completed.IsFaulted)
        {
            _ = completed.Exception;
        }
        else if (completed.Status == TaskStatus.RanToCompletion && completed.Result.IsSuccess)
        {
            completed.Result.Value.Dispose();
        }
    }
}
