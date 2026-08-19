using NTech.NVision.Domain.Jobs;
using NTech.NVision.Domain.Projects;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Core.Versioning;

namespace NTech.NVision.Domain.Tests;

public sealed class ProjectTests
{
    [Fact]
    public void AddJobAddsJobAndPreservesPrimaryWorkflowIdentity()
    {
        var project = new Project(ProjectId.New(), "Wire Marker", "WIRE_MARKER");
        WorkflowDefinition workflow = CreateWorkflow();
        var job = new Job(JobId.New(), "Top Inspection", "TOP", workflow, timeout: TimeSpan.FromSeconds(5));

        var result = project.AddJob(job);

        Assert.True(result.IsSuccess);
        Assert.Same(job, Assert.Single(project.Jobs));
        Assert.Equal(workflow.Id, job.WorkflowId);
    }

    [Fact]
    public void AddJobRejectsDuplicateIdentity()
    {
        var project = new Project(ProjectId.New(), "Wire Marker", "WIRE_MARKER");
        JobId id = JobId.New();
        project.AddJob(new Job(id, "Top", "TOP", CreateWorkflow()));

        var result = project.AddJob(new Job(id, "Bottom", "BOTTOM", CreateWorkflow()));

        Assert.True(result.IsFailure);
        Assert.Equal("PRJ-001", result.Error?.Code);
    }

    [Fact]
    public void AddJobRejectsCaseInsensitiveDuplicateCode()
    {
        var project = new Project(ProjectId.New(), "Wire Marker", "WIRE_MARKER");
        project.AddJob(new Job(JobId.New(), "Top", "TOP", CreateWorkflow()));

        var result = project.AddJob(new Job(JobId.New(), "Top Copy", "top", CreateWorkflow()));

        Assert.True(result.IsFailure);
        Assert.Equal("PRJ-002", result.Error?.Code);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    public void JobRejectsNonPositiveConfiguredTimeout(int milliseconds)
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new Job(
                JobId.New(),
                "Top",
                "TOP",
                CreateWorkflow(),
                timeout: TimeSpan.FromMilliseconds(milliseconds)));
    }

    [Fact]
    public void ProjectJobsCannotBeMutatedThroughPublicCollection()
    {
        var project = new Project(ProjectId.New(), "Wire Marker", "WIRE_MARKER");

        var collection = Assert.IsAssignableFrom<ICollection<Job>>(project.Jobs);

        Assert.Throws<NotSupportedException>(() =>
            collection.Add(new Job(JobId.New(), "Top", "TOP", CreateWorkflow())));
    }

    [Fact]
    public void AddRecipeRejectsCaseInsensitiveDuplicateCode()
    {
        var project = new Project(ProjectId.New(), "Wire Marker", "WIRE_MARKER");
        project.AddRecipe(CreateRecipe("TYPE_01"));

        var result = project.AddRecipe(CreateRecipe("type_01"));

        Assert.True(result.IsFailure);
        Assert.Equal("PRJ-004", result.Error?.Code);
    }

    [Fact]
    public void AddRecipeRejectsDuplicateIdentity()
    {
        var project = new Project(ProjectId.New(), "Wire Marker", "WIRE_MARKER");
        RecipeId id = RecipeId.New();
        project.AddRecipe(new Recipe(id, "Type 01", "TYPE_01", SemanticVersion.Parse("1.0.0")));

        var result = project.AddRecipe(
            new Recipe(id, "Type 02", "TYPE_02", SemanticVersion.Parse("1.0.0")));

        Assert.Equal("PRJ-003", result.Error?.Code);
    }

    private static WorkflowDefinition CreateWorkflow() =>
        new(WorkflowId.New(), "Inspection");

    private static Recipe CreateRecipe(string code) =>
        new(RecipeId.New(), code, code, SemanticVersion.Parse("1.0.0"));
}
