using NTech.NVision.Engine.AI.Models;

namespace NTech.NVision.Engine.AI.Inference;

public interface IAiInferenceBackend : IDisposable
{
    public string BackendId { get; }

    public string BackendVersion { get; }

    public ValueTask<InferenceOutput> RunAsync(
        ModelPackage package,
        PreparedTensor input,
        CancellationToken cancellationToken = default);
}
