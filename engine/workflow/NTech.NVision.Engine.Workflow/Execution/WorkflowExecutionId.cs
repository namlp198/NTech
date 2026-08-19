namespace NTech.NVision.Engine.Workflow.Execution;

public sealed record WorkflowExecutionId
{
    public WorkflowExecutionId(Guid value)
    {
        if (value == Guid.Empty)
        {
            throw new ArgumentException("Workflow execution identifier must not be empty.", nameof(value));
        }

        Value = value;
    }

    public Guid Value { get; }

    public static WorkflowExecutionId New() => new(Guid.NewGuid());

    public override string ToString() => Value.ToString("D");
}
