using System.Collections.ObjectModel;

namespace NTech.NVision.Contracts.Imaging;

public sealed class Frame
{
    private readonly ReadOnlyDictionary<string, string> metadata;

    public Frame(
        FrameId id,
        CameraId cameraId,
        long sequenceNumber,
        DateTimeOffset capturedAt,
        DateTimeOffset receivedAt,
        int width,
        int height,
        int strideBytes,
        PixelFormat pixelFormat,
        IFrameBuffer buffer,
        FrameStatus status = FrameStatus.Complete,
        FrameTimestampQuality timestampQuality = FrameTimestampQuality.HostOnly,
        IReadOnlyDictionary<string, string>? metadata = null)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(sequenceNumber);

        if (receivedAt < capturedAt)
        {
            throw new ArgumentException("Received timestamp must not precede captured timestamp.", nameof(receivedAt));
        }

        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(width);
        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(height);

        int minimumStride = checked(width * PixelFormatInfo.BytesPerPixel(pixelFormat));
        if (strideBytes < minimumStride)
        {
            throw new ArgumentOutOfRangeException(nameof(strideBytes), "Frame stride is smaller than one packed row.");
        }

        ArgumentNullException.ThrowIfNull(buffer);
        int requiredLength = checked(strideBytes * height);
        if (status == FrameStatus.Complete && buffer.Length < requiredLength)
        {
            throw new ArgumentException("A complete frame buffer is smaller than its declared dimensions.", nameof(buffer));
        }

        Id = id ?? throw new ArgumentNullException(nameof(id));
        CameraId = cameraId ?? throw new ArgumentNullException(nameof(cameraId));
        SequenceNumber = sequenceNumber;
        CapturedAt = capturedAt;
        ReceivedAt = receivedAt;
        Width = width;
        Height = height;
        StrideBytes = strideBytes;
        PixelFormat = pixelFormat;
        Buffer = buffer;
        Status = status;
        TimestampQuality = timestampQuality;
        this.metadata = new ReadOnlyDictionary<string, string>(
            metadata is null
                ? new Dictionary<string, string>(StringComparer.Ordinal)
                : new Dictionary<string, string>(metadata, StringComparer.Ordinal));
    }

    public FrameId Id { get; }

    public CameraId CameraId { get; }

    public long SequenceNumber { get; }

    public DateTimeOffset CapturedAt { get; }

    public DateTimeOffset ReceivedAt { get; }

    public int Width { get; }

    public int Height { get; }

    public int StrideBytes { get; }

    public PixelFormat PixelFormat { get; }

    public IFrameBuffer Buffer { get; }

    public FrameStatus Status { get; }

    public FrameTimestampQuality TimestampQuality { get; }

    public IReadOnlyDictionary<string, string> Metadata => metadata;
}
