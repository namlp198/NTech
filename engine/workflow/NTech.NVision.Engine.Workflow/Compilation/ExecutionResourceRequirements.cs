namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed record ExecutionResourceRequirements(
    bool UsesGpu,
    bool UsesCamera,
    bool UsesExternalIo,
    bool ContainsBlockingNodes);
