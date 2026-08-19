using System.Collections.Immutable;
using System.Diagnostics;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Contracts.Inspection;
using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;
using NTech.NVision.Engine.AI.Postprocessing;
using NTech.NVision.Engine.AI.Preprocessing;
using NTech.NVision.Engine.Acquisition.Services;
using NTech.NVision.Engine.Inspection.Storage;
using NTech.NVision.Engine.Inspection.Nodes;

namespace NTech.NVision.Engine.Inspection.WireMarker;

public sealed class WireMarkerInspectionEngine
{
    private readonly IAcquisitionService acquisition;
    private readonly IAiInferenceBackend backend;
    private readonly FileImageStore imageStore;
    private readonly IInspectionResultRepository repository;
    private readonly ModelSlotResolver slotResolver;
    private readonly TimeProvider timeProvider;
    private long sequence;

    public WireMarkerInspectionEngine(
        IAcquisitionService acquisition,
        ModelSlotResolver slotResolver,
        IAiInferenceBackend backend,
        IInspectionResultRepository repository,
        FileImageStore imageStore,
        TimeProvider? timeProvider = null)
    {
        this.acquisition = acquisition ?? throw new ArgumentNullException(nameof(acquisition));
        this.slotResolver = slotResolver ?? throw new ArgumentNullException(nameof(slotResolver));
        this.backend = backend ?? throw new ArgumentNullException(nameof(backend));
        this.repository = repository ?? throw new ArgumentNullException(nameof(repository));
        this.imageStore = imageStore ?? throw new ArgumentNullException(nameof(imageStore));
        this.timeProvider = timeProvider ?? TimeProvider.System;
    }

    public async ValueTask<InspectionResult> InspectAsync(
        WireMarkerInspectionRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(request);
        long itemSequence = Interlocked.Increment(ref sequence);
        DateTimeOffset startedAt = timeProvider.GetUtcNow();
        long totalStart = Stopwatch.GetTimestamp();
        string inspectionId = $"{request.StationCode}-{startedAt:yyyyMMddHHmmssfff}-{itemSequence:00000000}";
        string correlationId = request.CorrelationId ?? Guid.NewGuid().ToString("D");
        ResolvedModel? resolved = null;
        ImageEvidence? image = null;
        double acquisitionMs = 0;
        double preprocessingMs = 0;
        double inferenceMs = 0;
        double decisionMs = 0;
        double persistenceMs = 0;

        try
        {
            Result<ResolvedModel> modelResolution = slotResolver.Resolve(request.ModelSlot);
            if (modelResolution.IsFailure)
            {
                return await PersistTechnicalAsync(
                    modelResolution.Error!, "MODEL_RESOLUTION_FAILED", InspectionStatus.Error).ConfigureAwait(false);
            }

            resolved = modelResolution.Value;
            long step = Stopwatch.GetTimestamp();
            Result<FrameLease> acquired = await acquisition.AcquireAsync(
                request.CameraId, request.GrabRequest, cancellationToken).ConfigureAwait(false);
            acquisitionMs = Stopwatch.GetElapsedTime(step).TotalMilliseconds;
            if (acquired.IsFailure)
            {
                InspectionStatus status = acquired.Error!.Code switch
                {
                    "ACQ-002" => InspectionStatus.Timeout,
                    "ACQ-003" => InspectionStatus.Cancelled,
                    _ => InspectionStatus.Error,
                };
                string acquisitionDecisionCode = status == InspectionStatus.Timeout ? "CAMERA_TIMEOUT" : "CAMERA_ERROR";
                return await PersistTechnicalAsync(acquired.Error, acquisitionDecisionCode, status).ConfigureAwait(false);
            }

            using FrameLease lease = acquired.Value;
            step = Stopwatch.GetTimestamp();
            image = await imageStore.StoreAsync(inspectionId, lease.Frame, cancellationToken).ConfigureAwait(false);
            persistenceMs = Stopwatch.GetElapsedTime(step).TotalMilliseconds;

            WireMarkerWorkflowOutcome workflow = await WireMarkerWorkflowRunner.ExecuteAsync(
                lease.Frame,
                resolved.Package,
                backend,
                request.ExpectedClass,
                cancellationToken).ConfigureAwait(false);
            ClassificationPrediction prediction = workflow.Prediction;
            preprocessingMs = workflow.CropDuration.TotalMilliseconds;
            inferenceMs = workflow.ClassificationDuration.TotalMilliseconds;
            decisionMs = workflow.DecisionDuration.TotalMilliseconds;
            InspectionResult result = CreateResult(
                workflow.Status,
                workflow.DecisionCode,
                prediction.Confidence,
                [new ClassificationEvidence(
                    "marker_type",
                    prediction.ClassName,
                    prediction.ClassIndex,
                    prediction.Confidence,
                    prediction.Probabilities)],
                [image],
                []);
            await SaveValidatedAsync(result, cancellationToken).ConfigureAwait(false);
            return result;
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            return await PersistTechnicalAsync(
                new DomainError("INS-003", "Inspection was cancelled."),
                "CANCELLED",
                InspectionStatus.Cancelled).ConfigureAwait(false);
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            return await PersistTechnicalAsync(
                new DomainError("INS-002", $"Inspection processing failed: {exception.Message}"),
                "PROCESSING_ERROR",
                InspectionStatus.Error).ConfigureAwait(false);
        }

        async ValueTask<InspectionResult> PersistTechnicalAsync(
            DomainError error,
            string decisionCode,
            InspectionStatus status)
        {
            InspectionResult result = CreateResult(
                status,
                decisionCode,
                null,
                [],
                image is null ? [] : [image],
                [new InspectionError(error.Code, error.Message, "technical")]);
            await SaveValidatedAsync(result, CancellationToken.None).ConfigureAwait(false);
            return result;
        }

        InspectionResult CreateResult(
            InspectionStatus status,
            string decisionCode,
            double? score,
            ImmutableArray<ClassificationEvidence> classifications,
            ImmutableArray<ImageEvidence> images,
            ImmutableArray<InspectionError> errors)
        {
            DateTimeOffset completedAt = timeProvider.GetUtcNow();
            var timing = new InspectionTiming(
                acquisitionMs,
                preprocessingMs,
                inferenceMs,
                decisionMs,
                persistenceMs,
                Stopwatch.GetElapsedTime(totalStart).TotalMilliseconds);
            return new InspectionResult(
                "nvision-inspection-result",
                "1.0",
                inspectionId,
                correlationId,
                request.StationCode,
                itemSequence,
                startedAt,
                completedAt,
                status,
                decisionCode,
                score,
                request.ExpectedClass,
                classifications,
                images,
                errors,
                Trace(request, resolved),
                timing);
        }
    }

    private async ValueTask SaveValidatedAsync(
        InspectionResult result,
        CancellationToken cancellationToken)
    {
        InspectionResultValidator.Validate(result);
        await repository.SaveAsync(result, cancellationToken).ConfigureAwait(false);
    }

    private RuntimeTrace Trace(WireMarkerInspectionRequest request, ResolvedModel? model) => new(
        request.ProjectId,
        request.ProjectVersion,
        request.JobId,
        request.JobVersion,
        request.RecipeId,
        request.RecipeCode,
        request.RecipeVersion,
        request.CameraPlugin,
        request.CameraPluginVersion,
        backend.BackendId,
        backend.BackendVersion,
        model is null
            ? null
            : new ModelTrace(
                model.Slot.Value,
                model.Package.Manifest.Id.ToString(),
                model.Package.Manifest.Code,
                model.Package.Manifest.Version.ToString(),
                model.Package.ArtifactSha256));
}
