using System.Collections.ObjectModel;
using NTech.NVision.Core.Results;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Recipes;

public sealed class Recipe
{
    private readonly List<RecipeOverride> overrides = [];
    private readonly ReadOnlyCollection<RecipeOverride> overrideView;

    public Recipe(
        RecipeId id,
        string name,
        string code,
        SemanticVersion version,
        string? productCode = null,
        string? description = null,
        bool enabled = true,
        RecipeId? baseRecipeId = null)
    {
        Id = id ?? throw new ArgumentNullException(nameof(id));
        Name = DomainText.Require(name, nameof(name));
        Code = DomainText.Require(code, nameof(code), 128);
        Version = version ?? throw new ArgumentNullException(nameof(version));
        ProductCode = DomainText.Optional(productCode, 128);
        Description = DomainText.Optional(description);
        Enabled = enabled;
        BaseRecipeId = baseRecipeId;
        overrideView = overrides.AsReadOnly();
    }

    public RecipeId Id { get; }

    public string Name { get; }

    public string Code { get; }

    public SemanticVersion Version { get; }

    public string? ProductCode { get; }

    public string? Description { get; }

    public bool Enabled { get; }

    public RecipeId? BaseRecipeId { get; }

    public IReadOnlyList<RecipeOverride> Overrides => overrideView;

    public Result<RecipeOverride> AddOverride(RecipeOverride recipeOverride)
    {
        ArgumentNullException.ThrowIfNull(recipeOverride);

        if (overrides.Any(existing =>
            existing.TargetNodeId == recipeOverride.TargetNodeId &&
            existing.ParameterKey == recipeOverride.ParameterKey))
        {
            return Result.Failure<RecipeOverride>(
                RecipeErrors.DuplicateOverride(recipeOverride.TargetNodeId, recipeOverride.ParameterKey));
        }

        overrides.Add(recipeOverride);
        return Result.Success(recipeOverride);
    }
}
