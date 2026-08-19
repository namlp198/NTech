using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Engine.Acquisition.Imaging;

namespace NTech.NVision.Engine.Inspection.Nodes;

public static class CropNode
{
    public static FrameLease Execute(Frame source, int x, int y, int width, int height)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentOutOfRangeException.ThrowIfNegative(x);
        ArgumentOutOfRangeException.ThrowIfNegative(y);
        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(width);
        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(height);
        if (source.PixelFormat != PixelFormat.Mono8 || x + width > source.Width || y + height > source.Height)
        {
            throw new ArgumentOutOfRangeException(nameof(width), "Crop is outside the Mono8 source frame.");
        }

        var pixels = new byte[checked(width * height)];
        ReadOnlySpan<byte> sourcePixels = source.Buffer.Memory.Span;
        for (int row = 0; row < height; row++)
        {
            sourcePixels.Slice(((y + row) * source.StrideBytes) + x, width)
                .CopyTo(pixels.AsSpan(row * width, width));
        }

        var metadata = new Dictionary<string, string>(source.Metadata, StringComparer.Ordinal)
        {
            ["crop"] = $"{x},{y},{width},{height}",
        };
        var frame = new Frame(
            FrameId.New(),
            source.CameraId,
            source.SequenceNumber,
            source.CapturedAt,
            source.ReceivedAt,
            width,
            height,
            width,
            PixelFormat.Mono8,
            ManagedFrameBuffer.TakeOwnership(pixels),
            source.Status,
            source.TimestampQuality,
            metadata);
        return new FrameLease(frame);
    }
}
