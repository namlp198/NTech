namespace NTech.NVision.Core.Versioning;

public sealed record SemanticVersion : IComparable<SemanticVersion>
{
    public SemanticVersion(
        int major,
        int minor,
        int patch,
        string? preRelease = null,
        string? buildMetadata = null)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(major);
        ArgumentOutOfRangeException.ThrowIfNegative(minor);
        ArgumentOutOfRangeException.ThrowIfNegative(patch);

        Major = major;
        Minor = minor;
        Patch = patch;
        PreRelease = NormalizePreRelease(preRelease);
        BuildMetadata = NormalizeBuildMetadata(buildMetadata);
    }

    public int Major { get; }

    public int Minor { get; }

    public int Patch { get; }

    public string? PreRelease { get; }

    public string? BuildMetadata { get; }

    public static SemanticVersion Parse(string value)
    {
        if (!TryParse(value, out SemanticVersion? version))
        {
            throw new FormatException($"'{value}' is not a valid semantic version.");
        }

        return version!;
    }

    public static bool TryParse(string? value, out SemanticVersion? version)
    {
        version = null;
        if (string.IsNullOrWhiteSpace(value))
        {
            return false;
        }

        string[] versionAndMetadata = value.Trim().Split('+', 2);
        string[] coreAndPreRelease = versionAndMetadata[0].Split('-', 2);
        string[] core = coreAndPreRelease[0].Split('.');
        if (core.Length != 3 ||
            !TryParseCoreNumber(core[0], out int major) ||
            !TryParseCoreNumber(core[1], out int minor) ||
            !TryParseCoreNumber(core[2], out int patch))
        {
            return false;
        }

        string? preRelease = coreAndPreRelease.Length == 2 ? coreAndPreRelease[1] : null;
        if (!IsValidIdentifiers(preRelease) ||
            (versionAndMetadata.Length == 2 &&
             !IsValidIdentifiers(versionAndMetadata[1], rejectLeadingZeroNumericIdentifiers: false)))
        {
            return false;
        }

        string? buildMetadata = versionAndMetadata.Length == 2 ? versionAndMetadata[1] : null;
        version = new SemanticVersion(major, minor, patch, preRelease, buildMetadata);
        return true;
    }

    public int CompareTo(SemanticVersion? other)
    {
        if (other is null)
        {
            return 1;
        }

        int coreComparison = Major.CompareTo(other.Major);
        if (coreComparison == 0)
        {
            coreComparison = Minor.CompareTo(other.Minor);
        }

        if (coreComparison == 0)
        {
            coreComparison = Patch.CompareTo(other.Patch);
        }

        if (coreComparison != 0)
        {
            return coreComparison;
        }

        if (PreRelease is null)
        {
            return other.PreRelease is null ? 0 : 1;
        }

        if (other.PreRelease is null)
        {
            return -1;
        }

        return ComparePreRelease(PreRelease, other.PreRelease);
    }

    public static bool operator <(SemanticVersion? left, SemanticVersion? right) =>
        Compare(left, right) < 0;

    public static bool operator <=(SemanticVersion? left, SemanticVersion? right) =>
        Compare(left, right) <= 0;

    public static bool operator >(SemanticVersion? left, SemanticVersion? right) =>
        Compare(left, right) > 0;

    public static bool operator >=(SemanticVersion? left, SemanticVersion? right) =>
        Compare(left, right) >= 0;

    public override string ToString()
    {
        string value = PreRelease is null
            ? $"{Major}.{Minor}.{Patch}"
            : $"{Major}.{Minor}.{Patch}-{PreRelease}";
        return BuildMetadata is null ? value : $"{value}+{BuildMetadata}";
    }

    private static bool TryParseCoreNumber(string value, out int number)
    {
        if (value.Length > 1 && value[0] == '0')
        {
            number = default;
            return false;
        }

        return int.TryParse(value, System.Globalization.NumberStyles.None, System.Globalization.CultureInfo.InvariantCulture, out number) && number >= 0;
    }

    private static string? NormalizePreRelease(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        string normalized = value.Trim();
        if (!IsValidIdentifiers(normalized))
        {
            throw new ArgumentException("Pre-release contains an invalid semantic version identifier.", nameof(value));
        }

        return normalized;
    }

    private static string? NormalizeBuildMetadata(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        string normalized = value.Trim();
        if (!IsValidIdentifiers(normalized, rejectLeadingZeroNumericIdentifiers: false))
        {
            throw new ArgumentException("Build metadata contains an invalid semantic version identifier.", nameof(value));
        }

        return normalized;
    }

    private static bool IsValidIdentifiers(
        string? value,
        bool rejectLeadingZeroNumericIdentifiers = true)
    {
        if (value is null)
        {
            return true;
        }

        if (value.Length == 0)
        {
            return false;
        }

        foreach (string identifier in value.Split('.'))
        {
            if (identifier.Length == 0 || identifier.Any(character => !char.IsAsciiLetterOrDigit(character) && character != '-'))
            {
                return false;
            }

            if (rejectLeadingZeroNumericIdentifiers &&
                identifier.Length > 1 &&
                identifier[0] == '0' &&
                identifier.All(char.IsAsciiDigit))
            {
                return false;
            }
        }

        return true;
    }

    private static int ComparePreRelease(string left, string right)
    {
        string[] leftIdentifiers = left.Split('.');
        string[] rightIdentifiers = right.Split('.');
        int commonLength = Math.Min(leftIdentifiers.Length, rightIdentifiers.Length);

        for (int index = 0; index < commonLength; index++)
        {
            string leftIdentifier = leftIdentifiers[index];
            string rightIdentifier = rightIdentifiers[index];
            bool leftNumeric = leftIdentifier.All(char.IsAsciiDigit);
            bool rightNumeric = rightIdentifier.All(char.IsAsciiDigit);
            int comparison = leftNumeric && rightNumeric
                ? CompareNumericIdentifier(leftIdentifier, rightIdentifier)
                : leftNumeric
                    ? -1
                    : rightNumeric
                        ? 1
                        : string.CompareOrdinal(leftIdentifier, rightIdentifier);

            if (comparison != 0)
            {
                return comparison;
            }
        }

        return leftIdentifiers.Length.CompareTo(rightIdentifiers.Length);
    }

    private static int CompareNumericIdentifier(string left, string right)
    {
        int lengthComparison = left.Length.CompareTo(right.Length);
        return lengthComparison == 0 ? string.CompareOrdinal(left, right) : lengthComparison;
    }

    private static int Compare(SemanticVersion? left, SemanticVersion? right)
    {
        if (left is null)
        {
            return right is null ? 0 : -1;
        }

        return left.CompareTo(right);
    }
}
