using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;
using NTech.NVision.Core.Versioning;

namespace NTech.NVision.Engine.AI.Models;

public sealed class ModelRegistry
{
    private readonly Dictionary<(ModelId Id, SemanticVersion Version), ModelPackage> packages = [];

    public Result<ModelPackage> Register(ModelPackage package)
    {
        ArgumentNullException.ThrowIfNull(package);
        var key = (package.Manifest.Id, package.Manifest.Version);
        if (packages.TryGetValue(key, out ModelPackage? existing))
        {
            return string.Equals(existing.ArtifactSha256, package.ArtifactSha256, StringComparison.Ordinal)
                ? Result.Success(existing)
                : Failure("AIM-006", "A conflicting immutable model version is already registered.");
        }

        packages.Add(key, package);
        return Result.Success(package);
    }

    public Result<ModelPackage> Resolve(ModelId id, SemanticVersion version) =>
        packages.TryGetValue((id, version), out ModelPackage? package)
            ? Result.Success(package)
            : Failure("AIM-007", $"Model '{id}' version '{version}' is not registered.");

    private static Result<ModelPackage> Failure(string code, string message) =>
        Result.Failure<ModelPackage>(new DomainError(code, message));
}
