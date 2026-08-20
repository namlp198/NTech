namespace NTech.NVision.Native;

public sealed record NativeEngineInfo(
    uint AbiVersion,
    Version EngineVersion,
    ulong FeatureFlags);
