namespace NTech.NVision.Domain.Common;

internal static class DomainText
{
    public static string Require(string value, string parameterName, int maximumLength = 256)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new ArgumentException("Value is required.", parameterName);
        }

        string normalized = value.Trim();
        if (normalized.Length > maximumLength)
        {
            throw new ArgumentException($"Value must not exceed {maximumLength} characters.", parameterName);
        }

        return normalized;
    }

    public static string? Optional(string? value, int maximumLength = 2_000)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        string normalized = value.Trim();
        if (normalized.Length > maximumLength)
        {
            throw new ArgumentException($"Value must not exceed {maximumLength} characters.", nameof(value));
        }

        return normalized;
    }
}
