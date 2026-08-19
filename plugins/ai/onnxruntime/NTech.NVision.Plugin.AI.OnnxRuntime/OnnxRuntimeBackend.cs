using System.Collections.Concurrent;
using System.Collections.Immutable;
using Microsoft.ML.OnnxRuntime;
using Microsoft.ML.OnnxRuntime.Tensors;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;

namespace NTech.NVision.Plugin.AI.OnnxRuntime;

public sealed class OnnxRuntimeBackend : IAiInferenceBackend
{
    private readonly ConcurrentDictionary<string, Lazy<InferenceSession>> sessions = new(StringComparer.Ordinal);
    private int disposed;

    public string BackendId => "ntech.ai.onnxruntime";

    public string BackendVersion => typeof(InferenceSession).Assembly.GetName().Version?.ToString() ?? "unknown";

    public ValueTask<InferenceOutput> RunAsync(
        ModelPackage package,
        PreparedTensor input,
        CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
        ArgumentNullException.ThrowIfNull(package);
        ArgumentNullException.ThrowIfNull(input);
        cancellationToken.ThrowIfCancellationRequested();
        if (!string.Equals(package.Manifest.PreferredBackend, BackendId, StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                $"Model requires backend '{package.Manifest.PreferredBackend}', not '{BackendId}'.");
        }

        InferenceSession session = sessions.GetOrAdd(
            package.ArtifactSha256,
            _ => new Lazy<InferenceSession>(
                () => new InferenceSession(package.Artifact.ToArray()),
                LazyThreadSafetyMode.ExecutionAndPublication)).Value;
        var tensor = new DenseTensor<float>(input.Data.ToArray(), input.Dimensions.ToArray());
        using IDisposableReadOnlyCollection<DisposableNamedOnnxValue> outputs = session.Run(
            [NamedOnnxValue.CreateFromTensor(input.InputName, tensor)]);
        DisposableNamedOnnxValue output = outputs.Single(item =>
            string.Equals(item.Name, package.Manifest.OutputName, StringComparison.Ordinal));
        float[] values = output.AsTensor<float>().ToArray();
        cancellationToken.ThrowIfCancellationRequested();
        return ValueTask.FromResult(new InferenceOutput(output.Name, values.ToImmutableArray()));
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) != 0)
        {
            return;
        }

        foreach (Lazy<InferenceSession> session in sessions.Values)
        {
            if (session.IsValueCreated)
            {
                session.Value.Dispose();
            }
        }

        sessions.Clear();
    }
}
