using System.Collections.Immutable;

namespace NTech.NVision.Contracts.Inspection;

public sealed record ClassificationEvidence(
    string Key,
    string ClassName,
    int ClassIndex,
    double Confidence,
    ImmutableArray<double> Probabilities);

public sealed record ModelTrace(
    string Slot,
    string ModelId,
    string Code,
    string Version,
    string Checksum);

public sealed record RuntimeTrace(
    string ProjectId,
    string ProjectVersion,
    string JobId,
    string JobVersion,
    string RecipeId,
    string RecipeCode,
    string RecipeVersion,
    string CameraPlugin,
    string CameraPluginVersion,
    string BackendId,
    string BackendVersion,
    ModelTrace? Model);

public sealed record ImageEvidence(string Role, string Uri, string FrameId, string Checksum);

public sealed record InspectionError(string Code, string Message, string Category);

public sealed record InspectionTiming(
    double AcquisitionMs,
    double PreprocessingMs,
    double InferenceMs,
    double DecisionMs,
    double PersistenceMs,
    double TotalMs);

public sealed record InspectionResult(
    string Format,
    string SchemaVersion,
    string InspectionId,
    string CorrelationId,
    string StationCode,
    long Sequence,
    DateTimeOffset StartedAt,
    DateTimeOffset CompletedAt,
    InspectionStatus Status,
    string DecisionCode,
    double? Score,
    string ExpectedClass,
    ImmutableArray<ClassificationEvidence> Classifications,
    ImmutableArray<ImageEvidence> Images,
    ImmutableArray<InspectionError> Errors,
    RuntimeTrace Traceability,
    InspectionTiming Timing);
