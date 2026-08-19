using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

public abstract record DataValue
{
    protected DataValue(PortDataType dataType) =>
        DataType = dataType ?? throw new ArgumentNullException(nameof(dataType));

    public PortDataType DataType { get; }

    public static TypedDataValue<bool> FromBoolean(bool value) =>
        new(PortDataType.Boolean, value);

    public static TypedDataValue<long> FromInteger(long value) =>
        new(PortDataType.IntegerNumber, value);

    public static TypedDataValue<double> FromFloatingPoint(double value)
    {
        if (!double.IsFinite(value))
        {
            throw new ArgumentOutOfRangeException(nameof(value), "Floating-point data value must be finite.");
        }

        return new TypedDataValue<double>(PortDataType.FloatingPoint, value);
    }

    public static TypedDataValue<string> FromText(string value) =>
        new(PortDataType.Text, value ?? throw new ArgumentNullException(nameof(value)));
}

public sealed record TypedDataValue<T> : DataValue
{
    public TypedDataValue(PortDataType dataType, T value)
        : base(dataType)
    {
        ArgumentNullException.ThrowIfNull(value);
        Value = value;
    }

    public T Value { get; }
}
