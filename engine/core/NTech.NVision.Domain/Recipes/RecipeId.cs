using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Recipes;

public sealed record RecipeId
{
    public RecipeId(Guid value) => Value = StrongIdentifierGuard.RequireValue(value, nameof(value));

    public Guid Value { get; }

    public static RecipeId New() => new(Guid.NewGuid());

    public static RecipeId Parse(string value) => new(Guid.Parse(value));

    public override string ToString() => Value.ToString("D");
}
