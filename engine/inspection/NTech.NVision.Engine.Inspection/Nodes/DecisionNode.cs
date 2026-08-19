using NTech.NVision.Contracts.Inspection;

namespace NTech.NVision.Engine.Inspection.Nodes;

public static class DecisionNode
{
    public static (InspectionStatus Status, string Code) Execute(bool accepted) =>
        accepted ? (InspectionStatus.Ok, "OK") : (InspectionStatus.Ng, "WRONG_TYPE");
}
