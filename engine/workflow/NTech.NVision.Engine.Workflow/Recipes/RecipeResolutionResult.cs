using System.Collections.ObjectModel;
using NTech.NVision.Core.Validation;

namespace NTech.NVision.Engine.Workflow.Recipes;

public sealed class RecipeResolutionResult
{
    private readonly ReadOnlyCollection<ValidationIssue> issues;

    public RecipeResolutionResult(ResolvedRecipe? recipe, IEnumerable<ValidationIssue> issues)
    {
        ArgumentNullException.ThrowIfNull(issues);
        ValidationIssue[] snapshot = issues.ToArray();
        this.issues = Array.AsReadOnly(snapshot);
        Recipe = recipe;
    }

    public bool IsValid => issues.All(issue => issue.Severity != ValidationSeverity.Error);

    public ResolvedRecipe? Recipe { get; }

    public IReadOnlyList<ValidationIssue> Issues => issues;
}
