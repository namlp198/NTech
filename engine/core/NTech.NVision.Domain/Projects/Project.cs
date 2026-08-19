using System.Collections.ObjectModel;
using NTech.NVision.Core.Results;
using NTech.NVision.Domain.Common;
using NTech.NVision.Domain.Jobs;
using NTech.NVision.Domain.Recipes;

namespace NTech.NVision.Domain.Projects;

public sealed class Project
{
    private readonly List<Job> jobs = [];
    private readonly ReadOnlyCollection<Job> jobView;
    private readonly List<Recipe> recipes = [];
    private readonly ReadOnlyCollection<Recipe> recipeView;

    public Project(
        ProjectId id,
        string name,
        string code,
        string? description = null,
        int revision = 1,
        string schemaVersion = "1.0")
    {
        if (revision < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(revision), "Revision must be positive.");
        }

        Id = id ?? throw new ArgumentNullException(nameof(id));
        Name = DomainText.Require(name, nameof(name));
        Code = DomainText.Require(code, nameof(code), 128);
        Description = DomainText.Optional(description);
        Revision = revision;
        SchemaVersion = DomainText.Require(schemaVersion, nameof(schemaVersion), 64);
        jobView = jobs.AsReadOnly();
        recipeView = recipes.AsReadOnly();
    }

    public ProjectId Id { get; }

    public string Name { get; }

    public string Code { get; }

    public string? Description { get; }

    public int Revision { get; }

    public string SchemaVersion { get; }

    public IReadOnlyList<Job> Jobs => jobView;

    public IReadOnlyList<Recipe> Recipes => recipeView;

    public Result<Job> AddJob(Job job)
    {
        ArgumentNullException.ThrowIfNull(job);

        if (jobs.Any(existing => existing.Id == job.Id))
        {
            return Result.Failure<Job>(ProjectErrors.DuplicateJobId(job.Id));
        }

        if (jobs.Any(existing => string.Equals(existing.Code, job.Code, StringComparison.OrdinalIgnoreCase)))
        {
            return Result.Failure<Job>(ProjectErrors.DuplicateJobCode(job.Code));
        }

        jobs.Add(job);
        return Result.Success(job);
    }

    public Result<Recipe> AddRecipe(Recipe recipe)
    {
        ArgumentNullException.ThrowIfNull(recipe);

        if (recipes.Any(existing => existing.Id == recipe.Id))
        {
            return Result.Failure<Recipe>(ProjectErrors.DuplicateRecipeId(recipe.Id));
        }

        if (recipes.Any(existing => string.Equals(existing.Code, recipe.Code, StringComparison.OrdinalIgnoreCase)))
        {
            return Result.Failure<Recipe>(ProjectErrors.DuplicateRecipeCode(recipe.Code));
        }

        recipes.Add(recipe);
        return Result.Success(recipe);
    }
}
