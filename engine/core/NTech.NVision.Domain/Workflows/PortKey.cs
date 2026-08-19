using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record PortKey
{
    public PortKey(string value) => Value = StrongIdentifierGuard.RequireStableCode(value, nameof(value));

    public string Value { get; }

    public static PortKey Parse(string value) => new(value);

    public override string ToString() => Value;
}
