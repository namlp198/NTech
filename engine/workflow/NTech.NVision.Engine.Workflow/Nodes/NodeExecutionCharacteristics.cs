namespace NTech.NVision.Engine.Workflow.Nodes;

public sealed record NodeExecutionCharacteristics(
    NodeExecutionLifetime Lifetime,
    bool ThreadSafe = false,
    bool Reentrant = false,
    bool Deterministic = true,
    bool UsesGpu = false,
    bool UsesCamera = false,
    bool UsesExternalIo = false,
    bool MayBlock = false,
    bool SupportsCancellation = true,
    bool SupportsDebugPreview = false);
