using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record NodeTypeId
{
    public NodeTypeId(string value) => Value = StrongIdentifierGuard.RequireStableCode(value, nameof(value));

    public string Value { get; }

    public static NodeTypeId Parse(string value) => new(value);

    public override string ToString() => Value;
}
