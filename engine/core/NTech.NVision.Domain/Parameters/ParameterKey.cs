namespace NTech.NVision.Domain.Parameters;

public sealed record ParameterKey
{
    public ParameterKey(string value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new ArgumentException("Parameter key is required.", nameof(value));
        }

        string normalized = value.Trim();
        if (normalized.Length > 128)
        {
            throw new ArgumentException("Parameter key must not exceed 128 characters.", nameof(value));
        }

        if (!char.IsAsciiLetterOrDigit(normalized[0]) || !char.IsAsciiLetterOrDigit(normalized[^1]) ||
            normalized.Any(character => !char.IsAsciiLetterOrDigit(character) && character is not '.' and not '-' and not '_'))
        {
            throw new ArgumentException("Parameter key contains an unsupported character.", nameof(value));
        }

        Value = normalized;
    }

    public string Value { get; }

    public static ParameterKey Parse(string value) => new(value);

    public override string ToString() => Value;
}
