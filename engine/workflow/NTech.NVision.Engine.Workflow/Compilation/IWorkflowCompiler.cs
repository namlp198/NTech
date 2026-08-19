using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Recipes;

namespace NTech.NVision.Engine.Workflow.Compilation;

public interface IWorkflowCompiler
{
    public WorkflowCompilationResult Compile(
        WorkflowDefinition workflow,
        ResolvedRecipe resolvedRecipe);
}
