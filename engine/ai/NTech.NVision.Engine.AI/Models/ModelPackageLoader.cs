using System.Collections.Immutable;
using System.Security.Cryptography;
using System.Text.Json;
using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;
using NTech.NVision.Core.Versioning;

namespace NTech.NVision.Engine.AI.Models;

public static class ModelPackageLoader
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
    };

    public static async ValueTask<Result<ModelPackage>> LoadAsync(
        string packagePath,
        CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(packagePath))
        {
            throw new ArgumentException("Model package path is required.", nameof(packagePath));
        }

        string root = Path.GetFullPath(packagePath);
        try
        {
            if (!Directory.Exists(root) || !root.EndsWith(".nvmodel", StringComparison.OrdinalIgnoreCase))
            {
                return Failure("AIM-001", $"Model package directory is invalid: '{root}'.");
            }

            ManifestDocument document = await ReadJsonAsync<ManifestDocument>(
                Path.Combine(root, "manifest.json"), cancellationToken).ConfigureAwait(false);
            if (!string.Equals(document.Format, "nvision-ai-model", StringComparison.Ordinal) ||
                !string.Equals(document.SchemaVersion, "1.0", StringComparison.Ordinal) ||
                !string.Equals(document.Task, "classification", StringComparison.OrdinalIgnoreCase))
            {
                return Failure("AIM-002", "Model manifest format, schema, or task is unsupported.");
            }

            if (document.Input is null || document.Input.Width <= 0 || document.Input.Height <= 0 ||
                document.Input.Channels <= 0)
            {
                return Failure("AIM-002", "Model input contract is invalid.");
            }

            string artifactPath = ResolveWithin(root, document.Artifact);
            string labelsPath = ResolveWithin(root, document.Labels);
            string preprocessPath = ResolveWithin(root, document.Preprocess);
            string postprocessPath = ResolveWithin(root, document.Postprocess);
            string integrityPath = Path.Combine(root, "integrity.json");
            string encodedArtifact = await File.ReadAllTextAsync(artifactPath, cancellationToken).ConfigureAwait(false);
            byte[] artifact = Convert.FromBase64String(encodedArtifact.Trim());
            IntegrityDocument integrity = await ReadJsonAsync<IntegrityDocument>(integrityPath, cancellationToken)
                .ConfigureAwait(false);
            string checksum = Convert.ToHexString(SHA256.HashData(artifact)).ToLowerInvariant();
            if (!string.Equals(integrity.Algorithm, "sha256", StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(integrity.ArtifactSha256, checksum, StringComparison.OrdinalIgnoreCase))
            {
                return Failure("AIM-003", "Model artifact checksum does not match integrity.json.");
            }

            string[]? labels = await ReadJsonAsync<string[]>(labelsPath, cancellationToken).ConfigureAwait(false);
            if (labels is null || labels.Length < 2 || labels.Any(string.IsNullOrWhiteSpace))
            {
                return Failure("AIM-004", "Classification labels are invalid.");
            }

            PreprocessingDocument preprocessing = await ReadJsonAsync<PreprocessingDocument>(
                preprocessPath, cancellationToken).ConfigureAwait(false);
            PostprocessingDocument postprocessing = await ReadJsonAsync<PostprocessingDocument>(
                postprocessPath, cancellationToken).ConfigureAwait(false);
            if (!string.Equals(preprocessing.Operation, "vertical-half-means", StringComparison.Ordinal) ||
                !double.IsFinite(preprocessing.Scale) || preprocessing.Scale <= 0 ||
                !string.Equals(preprocessing.Layout, "features", StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(preprocessing.DataType, "float32", StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(postprocessing.Operation, "softmax-top1", StringComparison.Ordinal) ||
                postprocessing.TopK != 1)
            {
                return Failure("AIM-009", "Model preprocessing or postprocessing semantics are unsupported.");
            }

            var manifest = new ModelManifest(
                new ModelId(Guid.Parse(document.Id)),
                Required(document.Code, "code"),
                Required(document.Name, "name"),
                SemanticVersion.Parse(document.Version),
                document.Task,
                Required(document.Input.Name, "input.name"),
                document.Input.Width,
                document.Input.Height,
                document.Input.Channels,
                Required(document.Input.Layout, "input.layout"),
                Required(document.Input.DataType, "input.dataType"),
                Required(document.OutputName, "outputName"),
                Required(document.PreferredBackend, "preferredBackend"),
                document.SchemaVersion);
            return Result.Success(new ModelPackage(
                manifest,
                labels.ToImmutableArray(),
                new PreprocessingDefinition(
                    preprocessing.Operation,
                    preprocessing.Scale,
                    preprocessing.Layout,
                    preprocessing.DataType),
                new PostprocessingDefinition(postprocessing.Operation, postprocessing.TopK),
                artifact.ToImmutableArray(),
                checksum,
                root));
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException or
            JsonException or FormatException or ArgumentException)
        {
            return Failure("AIM-005", $"Model package load failed: {exception.Message}");
        }
    }

    private static async ValueTask<T> ReadJsonAsync<T>(string path, CancellationToken cancellationToken)
    {
        await using FileStream stream = File.OpenRead(path);
        T? value = await JsonSerializer.DeserializeAsync<T>(stream, JsonOptions, cancellationToken)
            .ConfigureAwait(false);
        return value ?? throw new JsonException($"'{path}' contains no JSON value.");
    }

    private static string ResolveWithin(string root, string relativePath)
    {
        string path = Path.GetFullPath(Path.Combine(root, Required(relativePath, "package path")));
        string prefix = root.EndsWith(Path.DirectorySeparatorChar)
            ? root
            : root + Path.DirectorySeparatorChar;
        if (!path.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException("Model package path escapes the package root.", nameof(relativePath));
        }

        return path;
    }

    private static string Required(string value, string name) =>
        string.IsNullOrWhiteSpace(value)
            ? throw new FormatException($"Model manifest '{name}' is required.")
            : value.Trim();

    private static Result<ModelPackage> Failure(string code, string message) =>
        Result.Failure<ModelPackage>(new DomainError(code, message));

    private sealed record ManifestDocument
    {
        public string Format { get; init; } = string.Empty;
        public string SchemaVersion { get; init; } = string.Empty;
        public string Id { get; init; } = string.Empty;
        public string Code { get; init; } = string.Empty;
        public string Name { get; init; } = string.Empty;
        public string Version { get; init; } = string.Empty;
        public string Task { get; init; } = string.Empty;
        public InputDocument? Input { get; init; }
        public string OutputName { get; init; } = string.Empty;
        public string PreferredBackend { get; init; } = string.Empty;
        public string Artifact { get; init; } = string.Empty;
        public string Labels { get; init; } = string.Empty;
        public string Preprocess { get; init; } = string.Empty;
        public string Postprocess { get; init; } = string.Empty;
    }

    private sealed record InputDocument
    {
        public string Name { get; init; } = string.Empty;
        public int Width { get; init; }
        public int Height { get; init; }
        public int Channels { get; init; }
        public string Layout { get; init; } = string.Empty;
        public string DataType { get; init; } = string.Empty;
    }

    private sealed record IntegrityDocument
    {
        public string Algorithm { get; init; } = string.Empty;
        public string ArtifactSha256 { get; init; } = string.Empty;
    }

    private sealed record PreprocessingDocument
    {
        public string Operation { get; init; } = string.Empty;
        public double Scale { get; init; }
        public string Layout { get; init; } = string.Empty;
        public string DataType { get; init; } = string.Empty;
    }

    private sealed record PostprocessingDocument
    {
        public string Operation { get; init; } = string.Empty;
        public int TopK { get; init; }
    }
}
