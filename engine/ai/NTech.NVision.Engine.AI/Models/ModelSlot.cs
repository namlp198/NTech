namespace NTech.NVision.Engine.AI.Models;

public sealed record ModelSlot
{
    public ModelSlot(string value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new ArgumentException("Model slot is required.", nameof(value));
        }

        Value = value.Trim();
    }

    public string Value { get; }

    public override string ToString() => Value;
}
