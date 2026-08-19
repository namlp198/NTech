using NTech.NVision.Core.Errors;
using NTech.NVision.Domain.Jobs;
using NTech.NVision.Domain.Recipes;

namespace NTech.NVision.Domain.Projects;

public static class ProjectErrors
{
    public static DomainError DuplicateJobId(JobId jobId) =>
        new("PRJ-001", $"Job '{jobId}' already exists in the Project.");

    public static DomainError DuplicateJobCode(string code) =>
        new("PRJ-002", $"Job code '{code}' already exists in the Project.");

    public static DomainError DuplicateRecipeId(RecipeId recipeId) =>
        new("PRJ-003", $"Recipe '{recipeId}' already exists in the Project.");

    public static DomainError DuplicateRecipeCode(string code) =>
        new("PRJ-004", $"Recipe code '{code}' already exists in the Project.");
}
