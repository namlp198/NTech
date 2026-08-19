using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed record CompiledInputBinding
{
    public CompiledInputBinding(PortKey targetPort, PortAddress source)
    {
        TargetPort = targetPort ?? throw new ArgumentNullException(nameof(targetPort));
        Source = source ?? throw new ArgumentNullException(nameof(source));
    }

    public PortKey TargetPort { get; }

    public PortAddress Source { get; }
}
