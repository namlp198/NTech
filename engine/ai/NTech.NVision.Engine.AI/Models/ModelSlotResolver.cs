using System.Collections.ObjectModel;
using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;

namespace NTech.NVision.Engine.AI.Models;

public sealed record ModelBinding(ModelSlot Slot, ModelId ModelId, Core.Versioning.SemanticVersion Version);

public sealed record ResolvedModel(ModelSlot Slot, ModelPackage Package);

public sealed class ModelSlotResolver
{
    private readonly ModelRegistry registry;
    private readonly ReadOnlyDictionary<ModelSlot, ModelBinding> bindings;

    public ModelSlotResolver(ModelRegistry registry, IEnumerable<ModelBinding> bindings)
    {
        this.registry = registry ?? throw new ArgumentNullException(nameof(registry));
        ArgumentNullException.ThrowIfNull(bindings);
        ModelBinding[] snapshot = bindings.ToArray();
        if (snapshot.GroupBy(binding => binding.Slot).Any(group => group.Count() > 1))
        {
            throw new ArgumentException("Model slots must be unique.", nameof(bindings));
        }

        this.bindings = new ReadOnlyDictionary<ModelSlot, ModelBinding>(
            snapshot.ToDictionary(binding => binding.Slot));
    }

    public Result<ResolvedModel> Resolve(ModelSlot slot)
    {
        ArgumentNullException.ThrowIfNull(slot);
        if (!bindings.TryGetValue(slot, out ModelBinding? binding))
        {
            return Result.Failure<ResolvedModel>(
                new DomainError("AIM-008", $"Model slot '{slot}' is not bound."));
        }

        Result<ModelPackage> package = registry.Resolve(binding.ModelId, binding.Version);
        return package.IsSuccess
            ? Result.Success(new ResolvedModel(slot, package.Value))
            : Result.Failure<ResolvedModel>(package.Error!);
    }
}
