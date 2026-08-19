using System.Collections.Immutable;

namespace NTech.NVision.Engine.AI.Models;

public sealed record ModelPackage(
    ModelManifest Manifest,
    ImmutableArray<string> Labels,
    PreprocessingDefinition Preprocessing,
    PostprocessingDefinition Postprocessing,
    ImmutableArray<byte> Artifact,
    string ArtifactSha256,
    string PackagePath);

public sealed record PreprocessingDefinition(
    string Operation,
    double Scale,
    string Layout,
    string DataType);

public sealed record PostprocessingDefinition(string Operation, int TopK);
