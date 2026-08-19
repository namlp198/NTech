using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record WorkflowId
{
    public WorkflowId(Guid value) => Value = StrongIdentifierGuard.RequireValue(value, nameof(value));

    public Guid Value { get; }

    public static WorkflowId New() => new(Guid.NewGuid());

    public static WorkflowId Parse(string value) => new(Guid.Parse(value));

    public override string ToString() => Value.ToString("D");
}
