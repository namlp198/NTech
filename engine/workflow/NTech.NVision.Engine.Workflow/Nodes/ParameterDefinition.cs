using NTech.NVision.Domain.Parameters;

namespace NTech.NVision.Engine.Workflow.Nodes;

public sealed class ParameterDefinition
{
    public ParameterDefinition(
        ParameterKey key,
        string displayName,
        ParameterValueKind dataType,
        ParameterValue? defaultValue = null,
        bool required = false,
        double? minimum = null,
        double? maximum = null,
        string? unit = null,
        bool recipeOverridable = false,
        bool runtimeBindable = false)
    {
        if (string.IsNullOrWhiteSpace(displayName))
        {
            throw new ArgumentException("Parameter display name is required.", nameof(displayName));
        }

        if (defaultValue is not null && defaultValue.Kind != dataType)
        {
            throw new ArgumentException("Default value type must match the parameter data type.", nameof(defaultValue));
        }

        if (minimum > maximum)
        {
            throw new ArgumentException("Parameter minimum must not exceed maximum.", nameof(minimum));
        }

        if ((minimum is not null || maximum is not null) &&
            dataType is not ParameterValueKind.WholeNumber and not ParameterValueKind.FloatingPoint)
        {
            throw new ArgumentException("Only numeric parameters may define minimum or maximum values.", nameof(dataType));
        }

        Key = key ?? throw new ArgumentNullException(nameof(key));
        DisplayName = displayName.Trim();
        DataType = dataType;
        DefaultValue = defaultValue;
        Required = required;
        Minimum = minimum;
        Maximum = maximum;
        Unit = string.IsNullOrWhiteSpace(unit) ? null : unit.Trim();
        RecipeOverridable = recipeOverridable;
        RuntimeBindable = runtimeBindable;
    }

    public ParameterKey Key { get; }

    public string DisplayName { get; }

    public ParameterValueKind DataType { get; }

    public ParameterValue? DefaultValue { get; }

    public bool Required { get; }

    public double? Minimum { get; }

    public double? Maximum { get; }

    public string? Unit { get; }

    public bool RecipeOverridable { get; }

    public bool RuntimeBindable { get; }

    public bool IsValid(ParameterValue value)
    {
        ArgumentNullException.ThrowIfNull(value);
        if (value.Kind != DataType)
        {
            return false;
        }

        double? numericValue = value.Kind switch
        {
            ParameterValueKind.WholeNumber => value.IntegerValue,
            ParameterValueKind.FloatingPoint => value.FloatingPointValue,
            _ => null,
        };

        return (Minimum is null || numericValue >= Minimum) &&
               (Maximum is null || numericValue <= Maximum);
    }
}
