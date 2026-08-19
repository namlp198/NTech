using NTech.NVision.Domain.Common;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Domain.Jobs;

public sealed class Job
{
    public Job(
        JobId id,
        string name,
        string code,
        WorkflowDefinition workflow,
        string? description = null,
        bool enabled = true,
        TimeSpan? timeout = null)
    {
        if (timeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(nameof(timeout), "Timeout must be positive when configured.");
        }

        Id = id ?? throw new ArgumentNullException(nameof(id));
        Name = DomainText.Require(name, nameof(name));
        Code = DomainText.Require(code, nameof(code), 128);
        Workflow = workflow ?? throw new ArgumentNullException(nameof(workflow));
        Description = DomainText.Optional(description);
        Enabled = enabled;
        Timeout = timeout;
    }

    public JobId Id { get; }

    public string Name { get; }

    public string Code { get; }

    public string? Description { get; }

    public WorkflowDefinition Workflow { get; }

    public WorkflowId WorkflowId => Workflow.Id;

    public bool Enabled { get; }

    public TimeSpan? Timeout { get; }
}
