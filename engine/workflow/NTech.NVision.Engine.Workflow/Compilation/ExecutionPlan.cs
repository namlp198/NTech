using System.Collections.ObjectModel;
using NTech.NVision.Core.Validation;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed class ExecutionPlan
{
    private readonly ReadOnlyCollection<ValidationIssue> diagnostics;
    private readonly ReadOnlyCollection<NodeId> entryNodes;
    private readonly ReadOnlyCollection<CompiledNode> nodes;
    private readonly ReadOnlyCollection<NodeId> terminalNodes;

    internal ExecutionPlan(
        PlanId id,
        WorkflowId workflowId,
        string workflowSchemaVersion,
        RecipeId recipeId,
        SemanticVersion recipeVersion,
        IEnumerable<CompiledNode> nodes,
        IEnumerable<NodeId> entryNodes,
        IEnumerable<NodeId> terminalNodes,
        ExecutionResourceRequirements resourceRequirements,
        IEnumerable<ValidationIssue> diagnostics,
        TimeSpan? timeout)
    {
        Id = id ?? throw new ArgumentNullException(nameof(id));
        WorkflowId = workflowId ?? throw new ArgumentNullException(nameof(workflowId));
        WorkflowSchemaVersion = workflowSchemaVersion ?? throw new ArgumentNullException(nameof(workflowSchemaVersion));
        RecipeId = recipeId ?? throw new ArgumentNullException(nameof(recipeId));
        RecipeVersion = recipeVersion ?? throw new ArgumentNullException(nameof(recipeVersion));
        this.nodes = Array.AsReadOnly(nodes?.ToArray() ?? throw new ArgumentNullException(nameof(nodes)));
        this.entryNodes = Array.AsReadOnly(entryNodes?.ToArray() ?? throw new ArgumentNullException(nameof(entryNodes)));
        this.terminalNodes = Array.AsReadOnly(terminalNodes?.ToArray() ?? throw new ArgumentNullException(nameof(terminalNodes)));
        ResourceRequirements = resourceRequirements ?? throw new ArgumentNullException(nameof(resourceRequirements));
        this.diagnostics = Array.AsReadOnly(diagnostics?.ToArray() ?? throw new ArgumentNullException(nameof(diagnostics)));
        Timeout = timeout;
    }

    public PlanId Id { get; }

    public WorkflowId WorkflowId { get; }

    public string WorkflowSchemaVersion { get; }

    public RecipeId RecipeId { get; }

    public SemanticVersion RecipeVersion { get; }

    public IReadOnlyList<CompiledNode> Nodes => nodes;

    public IReadOnlyList<NodeId> EntryNodes => entryNodes;

    public IReadOnlyList<NodeId> TerminalNodes => terminalNodes;

    public ExecutionResourceRequirements ResourceRequirements { get; }

    public IReadOnlyList<ValidationIssue> Diagnostics => diagnostics;

    public TimeSpan? Timeout { get; }
}
