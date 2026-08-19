namespace NTech.NVision.Contracts.Imaging;

public enum PixelFormat
{
    Mono8,
    Mono16,
    Rgb8,
    Bgr8,
}

public static class PixelFormatInfo
{
    public static int BytesPerPixel(PixelFormat pixelFormat) => pixelFormat switch
    {
        PixelFormat.Mono8 => 1,
        PixelFormat.Mono16 => 2,
        PixelFormat.Rgb8 => 3,
        PixelFormat.Bgr8 => 3,
        _ => throw new ArgumentOutOfRangeException(nameof(pixelFormat)),
    };
}
