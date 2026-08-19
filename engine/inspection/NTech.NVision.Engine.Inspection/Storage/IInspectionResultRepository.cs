using NTech.NVision.Contracts.Inspection;

namespace NTech.NVision.Engine.Inspection.Storage;

public interface IInspectionResultRepository
{
    public ValueTask InitializeAsync(CancellationToken cancellationToken = default);

    public ValueTask SaveAsync(InspectionResult result, CancellationToken cancellationToken = default);

    public ValueTask<InspectionResult?> GetAsync(
        string inspectionId,
        CancellationToken cancellationToken = default);
}
