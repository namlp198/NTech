namespace NTech.NVision.Contracts.Imaging;

public sealed record FrameId
{
    public FrameId(Guid value)
    {
        if (value == Guid.Empty)
        {
            throw new ArgumentException("Frame identifier must not be empty.", nameof(value));
        }

        Value = value;
    }

    public Guid Value { get; }

    public static FrameId New() => new(Guid.NewGuid());

    public override string ToString() => Value.ToString("D");
}
