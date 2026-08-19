using System.Collections.Immutable;

namespace NTech.NVision.Engine.AI.Inference;

public sealed record InferenceOutput(string OutputName, ImmutableArray<float> Values);
