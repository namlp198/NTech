namespace NTech.NVision.Domain.Parameters;

public sealed record ParameterValue
{
    private ParameterValue(
        ParameterValueKind kind,
        bool? booleanValue = null,
        long? integerValue = null,
        double? floatingPointValue = null,
        string? textValue = null)
    {
        Kind = kind;
        BooleanValue = booleanValue;
        IntegerValue = integerValue;
        FloatingPointValue = floatingPointValue;
        TextValue = textValue;
    }

    public ParameterValueKind Kind { get; }

    public bool? BooleanValue { get; }

    public long? IntegerValue { get; }

    public double? FloatingPointValue { get; }

    public string? TextValue { get; }

    public static ParameterValue FromBoolean(bool value) =>
        new(ParameterValueKind.Boolean, booleanValue: value);

    public static ParameterValue FromInteger(long value) =>
        new(ParameterValueKind.WholeNumber, integerValue: value);

    public static ParameterValue FromFloatingPoint(double value)
    {
        if (!double.IsFinite(value))
        {
            throw new ArgumentOutOfRangeException(nameof(value), "Floating-point parameter value must be finite.");
        }

        return new(ParameterValueKind.FloatingPoint, floatingPointValue: value);
    }

    public static ParameterValue FromText(string value) =>
        new(ParameterValueKind.Text, textValue: value ?? throw new ArgumentNullException(nameof(value)));
}
