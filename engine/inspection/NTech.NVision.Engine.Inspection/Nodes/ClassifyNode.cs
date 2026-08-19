using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;
using NTech.NVision.Engine.AI.Postprocessing;
using NTech.NVision.Engine.AI.Preprocessing;

namespace NTech.NVision.Engine.Inspection.Nodes;

public static class ClassifyNode
{
    public static async ValueTask<ClassificationPrediction> ExecuteAsync(
        Frame frame,
        ModelPackage package,
        IAiInferenceBackend backend,
        CancellationToken cancellationToken = default)
    {
        PreparedTensor input = ClassificationPreprocessor.Prepare(frame, package);
        InferenceOutput output = await backend.RunAsync(package, input, cancellationToken).ConfigureAwait(false);
        return ClassificationPostprocessor.Process(output, package);
    }
}
