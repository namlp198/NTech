using System.Collections.Immutable;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;

namespace NTech.NVision.Engine.AI.Preprocessing;

public static class ClassificationPreprocessor
{
    public static PreparedTensor Prepare(Frame frame, ModelPackage package)
    {
        ArgumentNullException.ThrowIfNull(frame);
        ArgumentNullException.ThrowIfNull(package);
        ModelManifest manifest = package.Manifest;
        if (frame.Status != FrameStatus.Complete || frame.PixelFormat != PixelFormat.Mono8)
        {
            throw new InvalidOperationException("Reference classification preprocessing requires a complete Mono8 frame.");
        }

        if (frame.Width != manifest.InputWidth || frame.Height != manifest.InputHeight ||
            !string.Equals(manifest.InputLayout, "FEATURES", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("Frame shape does not match the reference model input contract.");
        }

        ReadOnlySpan<byte> pixels = frame.Buffer.Memory.Span;
        int split = frame.Width / 2;
        long left = 0;
        long right = 0;
        int leftCount = 0;
        int rightCount = 0;
        for (int row = 0; row < frame.Height; row++)
        {
            int rowStart = row * frame.StrideBytes;
            for (int column = 0; column < frame.Width; column++)
            {
                if (column < split)
                {
                    left += pixels[rowStart + column];
                    leftCount++;
                }
                else
                {
                    right += pixels[rowStart + column];
                    rightCount++;
                }
            }
        }

        return new PreparedTensor(
            manifest.InputName,
            ImmutableArray.Create(1, 2),
            ImmutableArray.Create(
                (float)(left / (double)leftCount * package.Preprocessing.Scale),
                (float)(right / (double)rightCount * package.Preprocessing.Scale)));
    }
}
