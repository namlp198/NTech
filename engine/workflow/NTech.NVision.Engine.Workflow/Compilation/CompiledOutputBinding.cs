using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed record CompiledOutputBinding
{
    public CompiledOutputBinding(PortKey sourcePort, PortAddress target)
    {
        SourcePort = sourcePort ?? throw new ArgumentNullException(nameof(sourcePort));
        Target = target ?? throw new ArgumentNullException(nameof(target));
    }

    public PortKey SourcePort { get; }

    public PortAddress Target { get; }
}
