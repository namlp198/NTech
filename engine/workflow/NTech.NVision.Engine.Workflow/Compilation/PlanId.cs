namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed record PlanId
{
    public PlanId(Guid value)
    {
        if (value == Guid.Empty)
        {
            throw new ArgumentException("Plan identifier must not be empty.", nameof(value));
        }

        Value = value;
    }

    public Guid Value { get; }

    public static PlanId New() => new(Guid.NewGuid());

    public override string ToString() => Value.ToString("D");
}
