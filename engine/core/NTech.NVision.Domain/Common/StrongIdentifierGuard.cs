namespace NTech.NVision.Domain.Common;

internal static class StrongIdentifierGuard
{
    public static Guid RequireValue(Guid value, string parameterName)
    {
        if (value == Guid.Empty)
        {
            throw new ArgumentException("Identifier must not be empty.", parameterName);
        }

        return value;
    }

    public static string RequireStableCode(string value, string parameterName)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new ArgumentException("Identifier code is required.", parameterName);
        }

        string normalized = value.Trim();
        if (normalized.Length > 128)
        {
            throw new ArgumentException("Identifier code must not exceed 128 characters.", parameterName);
        }

        if (!string.Equals(normalized, normalized.ToLowerInvariant(), StringComparison.Ordinal))
        {
            throw new ArgumentException("Identifier code must use lowercase canonical form.", parameterName);
        }

        if (!char.IsAsciiLetterOrDigit(normalized[0]) || !char.IsAsciiLetterOrDigit(normalized[^1]))
        {
            throw new ArgumentException("Identifier code must start and end with an ASCII letter or digit.", parameterName);
        }

        foreach (char character in normalized)
        {
            if (!char.IsAsciiLetterOrDigit(character) && character is not '.' and not '-' and not '_')
            {
                throw new ArgumentException("Identifier code contains an unsupported character.", parameterName);
            }
        }

        return normalized;
    }
}
