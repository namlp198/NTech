namespace NTech.NVision.Plugin.Abstractions.Cameras;

public sealed record FrameGrabRequest
{
    public FrameGrabRequest(TimeSpan timeout, string frameRole = "source")
    {
        if (timeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(nameof(timeout), "Frame grab timeout must be positive.");
        }

        if (string.IsNullOrWhiteSpace(frameRole))
        {
            throw new ArgumentException("Frame role is required.", nameof(frameRole));
        }

        Timeout = timeout;
        FrameRole = frameRole.Trim();
    }

    public TimeSpan Timeout { get; }

    public string FrameRole { get; }
}
