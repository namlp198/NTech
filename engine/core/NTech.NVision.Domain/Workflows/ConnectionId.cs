using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record ConnectionId
{
    public ConnectionId(Guid value) => Value = StrongIdentifierGuard.RequireValue(value, nameof(value));

    public Guid Value { get; }

    public static ConnectionId New() => new(Guid.NewGuid());

    public static ConnectionId Parse(string value) => new(Guid.Parse(value));

    public override string ToString() => Value.ToString("D");
}
