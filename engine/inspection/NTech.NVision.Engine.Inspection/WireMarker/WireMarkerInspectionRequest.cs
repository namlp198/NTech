using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Engine.AI.Models;
using NTech.NVision.Plugin.Abstractions.Cameras;

namespace NTech.NVision.Engine.Inspection.WireMarker;

public sealed record WireMarkerInspectionRequest(
    CameraId CameraId,
    FrameGrabRequest GrabRequest,
    ModelSlot ModelSlot,
    string ExpectedClass,
    string StationCode,
    string ProjectId,
    string ProjectVersion,
    string JobId,
    string JobVersion,
    string RecipeId,
    string RecipeCode,
    string RecipeVersion,
    string CameraPlugin,
    string CameraPluginVersion,
    string? CorrelationId = null);
