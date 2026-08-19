using System.Collections.Immutable;

namespace NTech.NVision.Engine.AI.Postprocessing;

public sealed record ClassificationPrediction(
    string ClassName,
    int ClassIndex,
    double Confidence,
    ImmutableArray<double> Probabilities);
