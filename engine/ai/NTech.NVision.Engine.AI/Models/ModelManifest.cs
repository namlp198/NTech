using NTech.NVision.Core.Versioning;

namespace NTech.NVision.Engine.AI.Models;

public sealed record ModelManifest(
    ModelId Id,
    string Code,
    string Name,
    SemanticVersion Version,
    string Task,
    string InputName,
    int InputWidth,
    int InputHeight,
    int InputChannels,
    string InputLayout,
    string InputDataType,
    string OutputName,
    string PreferredBackend,
    string SchemaVersion);
