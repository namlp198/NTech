using System.Collections.Immutable;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;

namespace NTech.NVision.Engine.AI.Postprocessing;

public static class ClassificationPostprocessor
{
    public static ClassificationPrediction Process(InferenceOutput output, ModelPackage package)
    {
        ArgumentNullException.ThrowIfNull(output);
        ArgumentNullException.ThrowIfNull(package);
        if (!string.Equals(package.Postprocessing.Operation, "softmax-top1", StringComparison.Ordinal) ||
            package.Postprocessing.TopK != 1)
        {
            throw new InvalidOperationException("Classification postprocessing contract is unsupported.");
        }

        if (!string.Equals(output.OutputName, package.Manifest.OutputName, StringComparison.Ordinal) ||
            output.Values.Length != package.Labels.Length)
        {
            throw new InvalidOperationException("Inference output does not match the model classification contract.");
        }

        float maximum = output.Values.Max();
        double[] exponentials = output.Values.Select(value => Math.Exp(value - maximum)).ToArray();
        double total = exponentials.Sum();
        ImmutableArray<double> probabilities = exponentials.Select(value => value / total).ToImmutableArray();
        int classIndex = 0;
        for (int index = 1; index < probabilities.Length; index++)
        {
            if (probabilities[index] > probabilities[classIndex])
            {
                classIndex = index;
            }
        }

        return new ClassificationPrediction(
            package.Labels[classIndex],
            classIndex,
            probabilities[classIndex],
            probabilities);
    }
}
