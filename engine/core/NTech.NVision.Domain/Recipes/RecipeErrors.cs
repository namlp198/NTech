using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Domain.Recipes;

public static class RecipeErrors
{
    public static DomainError DuplicateOverride(NodeId nodeId, ParameterKey parameterKey) =>
        new("RCP-001", $"Recipe already overrides parameter '{parameterKey}' on node '{nodeId}'.");
}
