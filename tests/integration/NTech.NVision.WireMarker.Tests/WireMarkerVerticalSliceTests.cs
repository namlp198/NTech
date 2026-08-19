using System.Collections.Immutable;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Contracts.Inspection;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;
using NTech.NVision.Engine.Acquisition.Services;
using NTech.NVision.Engine.Inspection.Storage;
using NTech.NVision.Engine.Inspection.WireMarker;
using NTech.NVision.Plugin.Abstractions.Cameras;
using NTech.NVision.Plugin.AI.OnnxRuntime;
using NTech.NVision.Plugin.Camera.File;

namespace NTech.NVision.WireMarker.Tests;

public sealed class WireMarkerVerticalSliceTests
{
    private static readonly CameraId CameraId = new("wire-marker-file");
    private static readonly ModelSlot ClassifierSlot = new("marker_classifier");

    [Fact]
    public async Task Type01RecipeProducesOkThenWrongTypeProducesNgAndBothReconstructFromSqlite()
    {
        using var workspace = new TemporaryWorkspace();
        ModelPackage package = await LoadPackageAsync();
        ModelSlotResolver resolver = Resolver(package);
        await using var camera = new FileCameraDevice(
            CameraId,
            [Fixture("type01.pgm"), Fixture("type04.pgm")]);
        await using var acquisition = new AcquisitionService([camera]);
        Assert.True((await acquisition.ConnectAsync(CameraId)).IsSuccess);
        var repository = new SqliteInspectionResultRepository(workspace.DatabasePath);
        await repository.InitializeAsync();
        using var backend = new OnnxRuntimeBackend();
        var engine = new WireMarkerInspectionEngine(
            acquisition,
            resolver,
            backend,
            repository,
            new FileImageStore(workspace.ImageRoot),
            new FixedTimeProvider());

        InspectionResult accepted = await engine.InspectAsync(Request("TYPE_01"));
        InspectionResult rejected = await engine.InspectAsync(Request("TYPE_01"));

        Assert.Equal(InspectionStatus.Ok, accepted.Status);
        Assert.Equal("OK", accepted.DecisionCode);
        Assert.Equal("TYPE_01", Assert.Single(accepted.Classifications).ClassName);
        Assert.Equal(InspectionStatus.Ng, rejected.Status);
        Assert.Equal("WRONG_TYPE", rejected.DecisionCode);
        Assert.Equal("TYPE_04", Assert.Single(rejected.Classifications).ClassName);
        Assert.All([accepted, rejected], result =>
        {
            Assert.True(result.Score > 0.5);
            Assert.Empty(result.Errors);
            Assert.Equal("1.0.0", result.Traceability.Model?.Version);
            Assert.Equal("ntech.ai.onnxruntime", result.Traceability.BackendId);
            Assert.True(File.Exists(Assert.Single(result.Images).Uri));
        });

        await AssertReconstructedAsync(repository, accepted);
        await AssertReconstructedAsync(repository, rejected);
    }

    [Fact]
    public async Task BackendFailureIsPersistedAsErrorAndNeverMappedToNg()
    {
        using var workspace = new TemporaryWorkspace();
        ModelPackage package = await LoadPackageAsync();
        await using var camera = new FileCameraDevice(CameraId, [Fixture("type01.pgm")]);
        await using var acquisition = new AcquisitionService([camera]);
        Assert.True((await acquisition.ConnectAsync(CameraId)).IsSuccess);
        var repository = new SqliteInspectionResultRepository(workspace.DatabasePath);
        await repository.InitializeAsync();
        using var backend = new ThrowingBackend();
        var engine = new WireMarkerInspectionEngine(
            acquisition,
            Resolver(package),
            backend,
            repository,
            new FileImageStore(workspace.ImageRoot));

        InspectionResult result = await engine.InspectAsync(Request("TYPE_01"));

        Assert.Equal(InspectionStatus.Error, result.Status);
        Assert.Equal("PROCESSING_ERROR", result.DecisionCode);
        Assert.Empty(result.Classifications);
        Assert.Equal("INS-002", Assert.Single(result.Errors).Code);
        await AssertReconstructedAsync(repository, result);
    }

    [Fact]
    public async Task ModelPackageChecksumAndSlotResolveToExactImmutableVersion()
    {
        ModelPackage package = await LoadPackageAsync();
        var registry = new ModelRegistry();

        Assert.True(registry.Register(package).IsSuccess);
        ResolvedModel resolved = Resolver(package).Resolve(ClassifierSlot).Value;

        Assert.Same(package, resolved.Package);
        Assert.Equal("3bec1d7190001d1dc1f4ef009013b1abafb1d6a7d725840f15c54920e639e313", package.ArtifactSha256);
        Assert.Equal(["TYPE_01", "TYPE_04"], package.Labels.ToArray());
    }

    [Fact]
    public async Task RepeatedInspectionReusesBackendWithoutBreakingFrameOwnership()
    {
        using var workspace = new TemporaryWorkspace();
        ModelPackage package = await LoadPackageAsync();
        await using var camera = new FileCameraDevice(CameraId, [Fixture("type01.pgm")], loop: true);
        await using var acquisition = new AcquisitionService([camera]);
        Assert.True((await acquisition.ConnectAsync(CameraId)).IsSuccess);
        var repository = new SqliteInspectionResultRepository(workspace.DatabasePath);
        await repository.InitializeAsync();
        using var backend = new OnnxRuntimeBackend();
        var engine = new WireMarkerInspectionEngine(
            acquisition,
            Resolver(package),
            backend,
            repository,
            new FileImageStore(workspace.ImageRoot));

        var results = new List<InspectionResult>();
        for (int index = 0; index < 20; index++)
        {
            results.Add(await engine.InspectAsync(Request("TYPE_01")));
        }

        Assert.All(results, result => Assert.Equal(InspectionStatus.Ok, result.Status));
        Assert.Equal(20, results.Select(result => result.InspectionId).Distinct(StringComparer.Ordinal).Count());
        Assert.Equal(20, Directory.GetFiles(workspace.ImageRoot, "*.pgm", SearchOption.AllDirectories).Length);
    }

    private static async Task<ModelPackage> LoadPackageAsync()
    {
        var result = await ModelPackageLoader.LoadAsync(Path.Combine(
            AppContext.BaseDirectory,
            "WireMarker",
            "models",
            "wire-marker-1.0.0.nvmodel"));
        Assert.True(result.IsSuccess, result.Error?.Message);
        return result.Value;
    }

    private static ModelSlotResolver Resolver(ModelPackage package)
    {
        var registry = new ModelRegistry();
        Assert.True(registry.Register(package).IsSuccess);
        return new ModelSlotResolver(
            registry,
            [new ModelBinding(ClassifierSlot, package.Manifest.Id, package.Manifest.Version)]);
    }

    private static WireMarkerInspectionRequest Request(string expectedClass) => new(
        CameraId,
        new FrameGrabRequest(TimeSpan.FromSeconds(2), "wire-marker"),
        ClassifierSlot,
        expectedClass,
        "WM01",
        "11111111-1111-1111-1111-111111111111",
        "1.0.0",
        "22222222-2222-2222-2222-222222222222",
        "1.0.0",
        "33333333-3333-3333-3333-333333333333",
        "TYPE_01",
        "1.0.0",
        "ntech.camera.file",
        "1.0.0");

    private static string Fixture(string name) =>
        Path.Combine(AppContext.BaseDirectory, "WireMarker", "fixtures", name);

    private static async Task AssertReconstructedAsync(
        SqliteInspectionResultRepository repository,
        InspectionResult expected)
    {
        InspectionResult? loaded = await repository.GetAsync(expected.InspectionId);
        Assert.NotNull(loaded);
        Assert.Equal(expected.InspectionId, loaded.InspectionId);
        Assert.Equal(expected.Status, loaded.Status);
        Assert.Equal(expected.DecisionCode, loaded.DecisionCode);
        Assert.Equal(expected.Traceability, loaded.Traceability);
        Assert.Equal(expected.Classifications.Length, loaded.Classifications.Length);
        for (int index = 0; index < expected.Classifications.Length; index++)
        {
            ClassificationEvidence expectedClassification = expected.Classifications[index];
            ClassificationEvidence loadedClassification = loaded.Classifications[index];
            Assert.Equal(expectedClassification.Key, loadedClassification.Key);
            Assert.Equal(expectedClassification.ClassName, loadedClassification.ClassName);
            Assert.Equal(expectedClassification.ClassIndex, loadedClassification.ClassIndex);
            Assert.Equal(expectedClassification.Confidence, loadedClassification.Confidence);
            Assert.Equal(
                expectedClassification.Probabilities.ToArray(),
                loadedClassification.Probabilities.ToArray());
        }
        Assert.Equal(expected.Images.ToArray(), loaded.Images.ToArray());
        Assert.Equal(expected.Errors.ToArray(), loaded.Errors.ToArray());
    }

    private sealed class ThrowingBackend : IAiInferenceBackend
    {
        public string BackendId => "ntech.ai.onnxruntime";

        public string BackendVersion => "test-failure";

        public ValueTask<InferenceOutput> RunAsync(
            ModelPackage package,
            PreparedTensor input,
            CancellationToken cancellationToken = default) =>
            ValueTask.FromException<InferenceOutput>(new InvalidOperationException("Injected model failure."));

        public void Dispose()
        {
        }
    }

    private sealed class FixedTimeProvider : TimeProvider
    {
        public override DateTimeOffset GetUtcNow() => new(2026, 8, 19, 0, 0, 0, TimeSpan.Zero);
    }

    private sealed class TemporaryWorkspace : IDisposable
    {
        public TemporaryWorkspace()
        {
            Root = Path.Combine(Path.GetTempPath(), "nvision-wire-marker-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Root);
        }

        public string Root { get; }

        public string DatabasePath => Path.Combine(Root, "inspection.db");

        public string ImageRoot => Path.Combine(Root, "images");

        public void Dispose() => Directory.Delete(Root, recursive: true);
    }
}
