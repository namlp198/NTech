using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Jobs;

public sealed record JobId
{
    public JobId(Guid value) => Value = StrongIdentifierGuard.RequireValue(value, nameof(value));

    public Guid Value { get; }

    public static JobId New() => new(Guid.NewGuid());

    public static JobId Parse(string value) => new(Guid.Parse(value));

    public override string ToString() => Value.ToString("D");
}
