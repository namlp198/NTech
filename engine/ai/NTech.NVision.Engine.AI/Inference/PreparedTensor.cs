using System.Collections.Immutable;

namespace NTech.NVision.Engine.AI.Inference;

public sealed record PreparedTensor(
    string InputName,
    ImmutableArray<int> Dimensions,
    ImmutableArray<float> Data);
