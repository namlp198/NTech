using NTech.NVision.Engine.AI.Postprocessing;

namespace NTech.NVision.Engine.Inspection.Nodes;

public static class CompareNode
{
    public static bool Execute(ClassificationPrediction prediction, string expectedClass)
    {
        ArgumentNullException.ThrowIfNull(prediction);
        ArgumentException.ThrowIfNullOrWhiteSpace(expectedClass);
        return string.Equals(prediction.ClassName, expectedClass, StringComparison.Ordinal);
    }
}
