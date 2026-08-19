namespace NTech.NVision.Domain.Parameters;

public sealed record NodeParameterValue
{
    public NodeParameterValue(ParameterKey key, ParameterValue value, ParameterValueSource source)
    {
        Key = key ?? throw new ArgumentNullException(nameof(key));
        Value = value ?? throw new ArgumentNullException(nameof(value));
        Source = source;
    }

    public ParameterKey Key { get; }

    public ParameterValue Value { get; }

    public ParameterValueSource Source { get; }
}
