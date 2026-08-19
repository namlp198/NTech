using System.Security.Cryptography;
using System.Text;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;
using NTech.NVision.Engine.Acquisition.Imaging;
using NTech.NVision.Plugin.Abstractions.Cameras;

namespace NTech.NVision.Plugin.Camera.File;

public sealed class FileCameraDevice : ICameraDevice
{
    private readonly SemaphoreSlim gate = new(1, 1);
    private readonly bool loop;
    private readonly string[] sources;
    private readonly TimeProvider timeProvider;
    private int nextSourceIndex;
    private long sequenceNumber;
    private int disposed;
    private int state = (int)CameraDeviceState.Disconnected;

    public FileCameraDevice(
        CameraId id,
        IEnumerable<string> sources,
        bool loop = false,
        TimeProvider? timeProvider = null)
    {
        Id = id ?? throw new ArgumentNullException(nameof(id));
        ArgumentNullException.ThrowIfNull(sources);
        this.sources = sources
            .Select(source => string.IsNullOrWhiteSpace(source)
                ? throw new ArgumentException("File Camera source paths are required.", nameof(sources))
                : Path.GetFullPath(source))
            .ToArray();
        if (this.sources.Length == 0)
        {
            throw new ArgumentException("File Camera requires at least one source.", nameof(sources));
        }

        this.loop = loop;
        this.timeProvider = timeProvider ?? TimeProvider.System;
    }

    public CameraId Id { get; }

    public CameraDeviceState State => (CameraDeviceState)Volatile.Read(ref state);

    public async ValueTask<Result<CameraDeviceState>> ConnectAsync(
        CancellationToken cancellationToken = default)
    {
        await gate.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            if (State == CameraDeviceState.Disposed)
            {
                return FailureState("FILECAM-005", $"File Camera '{Id}' is disposed.");
            }

            Volatile.Write(ref state, (int)CameraDeviceState.Connecting);
            string? missing = sources.FirstOrDefault(source => !System.IO.File.Exists(source));
            if (missing is not null)
            {
                Volatile.Write(ref state, (int)CameraDeviceState.Faulted);
                return FailureState("FILECAM-002", $"File Camera source does not exist: '{missing}'.");
            }

            nextSourceIndex = 0;
            sequenceNumber = 0;
            Volatile.Write(ref state, (int)CameraDeviceState.Connected);
            return Result.Success(CameraDeviceState.Connected);
        }
        finally
        {
            gate.Release();
        }
    }

    public async ValueTask<Result<FrameLease>> GrabAsync(
        FrameGrabRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(request);
        await gate.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            if (State != CameraDeviceState.Connected)
            {
                return Result.Failure<FrameLease>(new DomainError(
                    "FILECAM-001",
                    $"File Camera '{Id}' must be connected before grabbing a frame."));
            }

            if (nextSourceIndex >= sources.Length)
            {
                if (!loop)
                {
                    return Result.Failure<FrameLease>(new DomainError(
                        "FILECAM-003",
                        $"File Camera '{Id}' reached the end of its source sequence."));
                }

                nextSourceIndex = 0;
            }

            string source = sources[nextSourceIndex++];
            string content = await System.IO.File.ReadAllTextAsync(source, cancellationToken).ConfigureAwait(false);
            PortableGraymap image;
            try
            {
                image = PortableGraymapReader.Parse(content);
            }
            catch (Exception exception) when (exception is FormatException or OverflowException)
            {
                return Result.Failure<FrameLease>(new DomainError(
                    "FILECAM-004",
                    $"File Camera source '{source}' is invalid: {exception.Message}"));
            }

            long sequence = checked(++sequenceNumber);
            DateTimeOffset capturedAt = timeProvider.GetUtcNow();
            var buffer = ManagedFrameBuffer.TakeOwnership(image.Pixels);
            var frame = new Frame(
                CreateFrameId(Id, Path.GetFileName(source), sequence),
                Id,
                sequence,
                capturedAt,
                capturedAt,
                image.Width,
                image.Height,
                image.Width,
                PixelFormat.Mono8,
                buffer,
                metadata: new Dictionary<string, string>(StringComparer.Ordinal)
                {
                    ["sourceFile"] = Path.GetFileName(source),
                    ["sourcePath"] = source,
                    ["frameRole"] = request.FrameRole,
                });
            return Result.Success(new FrameLease(frame));
        }
        catch (IOException exception)
        {
            return Result.Failure<FrameLease>(new DomainError(
                "FILECAM-006",
                $"File Camera '{Id}' could not read its source: {exception.Message}"));
        }
        finally
        {
            gate.Release();
        }
    }

    public async ValueTask<Result<CameraDeviceState>> DisconnectAsync(
        CancellationToken cancellationToken = default)
    {
        await gate.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            if (State == CameraDeviceState.Disposed)
            {
                return FailureState("FILECAM-005", $"File Camera '{Id}' is disposed.");
            }

            Volatile.Write(ref state, (int)CameraDeviceState.Disconnected);
            return Result.Success(CameraDeviceState.Disconnected);
        }
        finally
        {
            gate.Release();
        }
    }

    public async ValueTask DisposeAsync()
    {
        if (Interlocked.Exchange(ref disposed, 1) != 0)
        {
            return;
        }

        await gate.WaitAsync(CancellationToken.None).ConfigureAwait(false);
        try
        {
            Volatile.Write(ref state, (int)CameraDeviceState.Disposed);
        }
        finally
        {
            gate.Release();
            gate.Dispose();
        }
    }

    private static Result<CameraDeviceState> FailureState(string code, string message) =>
        Result.Failure<CameraDeviceState>(new DomainError(code, message));

    private static FrameId CreateFrameId(CameraId cameraId, string sourceName, long sequence)
    {
        byte[] identity = Encoding.UTF8.GetBytes($"{cameraId.Value}|{sourceName}|{sequence}");
        byte[] hash = SHA256.HashData(identity);
        return new FrameId(new Guid(hash.AsSpan(0, 16)));
    }
}
