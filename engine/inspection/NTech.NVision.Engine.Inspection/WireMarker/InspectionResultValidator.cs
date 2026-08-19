using NTech.NVision.Contracts.Inspection;

namespace NTech.NVision.Engine.Inspection.WireMarker;

public static class InspectionResultValidator
{
    public static void Validate(InspectionResult result)
    {
        ArgumentNullException.ThrowIfNull(result);
        if (result.CompletedAt < result.StartedAt)
        {
            throw new InvalidOperationException("Inspection completion must not precede its start.");
        }

        if (result.Status is InspectionStatus.Ok or InspectionStatus.Ng)
        {
            if (result.Classifications.Length != 1 || result.Errors.Length != 0 || result.Score is null)
            {
                throw new InvalidOperationException("A quality result requires one classification and no technical error.");
            }
        }
        else if (result.Status is InspectionStatus.Error or InspectionStatus.Timeout or InspectionStatus.Cancelled)
        {
            if (result.Errors.Length == 0 || result.DecisionCode == "OK" || result.DecisionCode == "WRONG_TYPE")
            {
                throw new InvalidOperationException("A technical result requires an explicit technical error code.");
            }
        }

        if (result.Status == InspectionStatus.Ok && result.DecisionCode != "OK")
        {
            throw new InvalidOperationException("OK status requires the OK decision code.");
        }

        if (result.Status == InspectionStatus.Ng && result.DecisionCode == "OK")
        {
            throw new InvalidOperationException("NG status cannot use the OK decision code.");
        }
    }
}
