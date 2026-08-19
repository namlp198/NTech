using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record NodeId
{
    public NodeId(Guid value) => Value = StrongIdentifierGuard.RequireValue(value, nameof(value));

    public Guid Value { get; }

    public static NodeId New() => new(Guid.NewGuid());

    public static NodeId Parse(string value) => new(Guid.Parse(value));

    public override string ToString() => Value.ToString("D");
}
