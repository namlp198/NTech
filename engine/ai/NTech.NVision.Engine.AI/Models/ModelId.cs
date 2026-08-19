namespace NTech.NVision.Engine.AI.Models;

public sealed record ModelId
{
    public ModelId(Guid value)
    {
        if (value == Guid.Empty)
        {
            throw new ArgumentException("Model identifier must not be empty.", nameof(value));
        }

        Value = value;
    }

    public Guid Value { get; }

    public override string ToString() => Value.ToString("D");
}
