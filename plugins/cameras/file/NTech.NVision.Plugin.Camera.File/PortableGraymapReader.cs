using System.Globalization;
using System.Text;

namespace NTech.NVision.Plugin.Camera.File;

internal static class PortableGraymapReader
{
    public static PortableGraymap Parse(string content)
    {
        ArgumentNullException.ThrowIfNull(content);
        string[] tokens = Tokenize(content).ToArray();
        if (tokens.Length < 4 || !string.Equals(tokens[0], "P2", StringComparison.Ordinal))
        {
            throw new FormatException("Only ASCII PGM (P2) images are supported by the initial File Camera.");
        }

        int width = ParsePositive(tokens[1], "width");
        int height = ParsePositive(tokens[2], "height");
        int maximum = ParsePositive(tokens[3], "maximum value");
        if (maximum > byte.MaxValue)
        {
            throw new FormatException("PGM maximum value must not exceed 255.");
        }

        int pixelCount = checked(width * height);
        if (tokens.Length != pixelCount + 4)
        {
            throw new FormatException($"PGM declares {pixelCount} pixels but contains {tokens.Length - 4}.");
        }

        var pixels = new byte[pixelCount];
        for (int index = 0; index < pixelCount; index++)
        {
            int value = int.Parse(tokens[index + 4], NumberStyles.None, CultureInfo.InvariantCulture);
            if (value < 0 || value > maximum)
            {
                throw new FormatException($"PGM pixel {index} is outside the declared range.");
            }

            pixels[index] = (byte)((value * byte.MaxValue + (maximum / 2)) / maximum);
        }

        return new PortableGraymap(width, height, pixels);
    }

    private static IEnumerable<string> Tokenize(string content)
    {
        var token = new StringBuilder();
        bool comment = false;
        foreach (char character in content)
        {
            if (comment)
            {
                if (character is '\r' or '\n')
                {
                    comment = false;
                }

                continue;
            }

            if (character == '#')
            {
                if (token.Length > 0)
                {
                    yield return token.ToString();
                    token.Clear();
                }

                comment = true;
                continue;
            }

            if (char.IsWhiteSpace(character))
            {
                if (token.Length > 0)
                {
                    yield return token.ToString();
                    token.Clear();
                }

                continue;
            }

            token.Append(character);
        }

        if (token.Length > 0)
        {
            yield return token.ToString();
        }
    }

    private static int ParsePositive(string value, string name)
    {
        if (!int.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out int parsed) || parsed <= 0)
        {
            throw new FormatException($"PGM {name} must be a positive integer.");
        }

        return parsed;
    }
}

internal sealed record PortableGraymap(int Width, int Height, byte[] Pixels);
