using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Recipes;
using NTech.NVision.Engine.Workflow.Registry;
using NTech.NVision.Engine.Workflow.Validation;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed class WorkflowCompiler : IWorkflowCompiler
{
    private static readonly IComparer<NodeId> NodeIdComparer =
        Comparer<NodeId>.Create((left, right) => left.Value.CompareTo(right.Value));

    private readonly INodeTypeRegistry registry;
    private readonly IWorkflowValidator validator;

    public WorkflowCompiler(INodeTypeRegistry registry, IWorkflowValidator? validator = null)
    {
        this.registry = registry ?? throw new ArgumentNullException(nameof(registry));
        this.validator = validator ?? new WorkflowValidator(registry);
    }

    public WorkflowCompilationResult Compile(
        WorkflowDefinition workflow,
        ResolvedRecipe resolvedRecipe)
    {
        ArgumentNullException.ThrowIfNull(workflow);
        ArgumentNullException.ThrowIfNull(resolvedRecipe);

        WorkflowValidationResult validation = validator.Validate(workflow);
        var issues = validation.Diagnostics.Select(diagnostic => diagnostic.Issue).ToList();
        if (!validation.IsValid)
        {
            return new WorkflowCompilationResult(null, issues);
        }

        WorkflowNode[] enabledNodes = workflow.Nodes.Where(node => node.Enabled).ToArray();
        var enabledNodeIds = enabledNodes.Select(node => node.Id).ToHashSet();
        WorkflowConnection[] enabledConnections = workflow.Connections
            .Where(connection =>
                enabledNodeIds.Contains(connection.SourceNodeId) &&
                enabledNodeIds.Contains(connection.TargetNodeId))
            .ToArray();
        var definitions = new Dictionary<NodeId, NodeTypeDefinition>();
        var boundParameters = new Dictionary<NodeId, IReadOnlyDictionary<ParameterKey, NodeParameterValue>>();

        foreach (WorkflowNode node in enabledNodes)
        {
            var definitionResult = registry.Resolve(node.NodeTypeId, node.NodeTypeVersion);
            if (definitionResult.IsFailure)
            {
                issues.Add(Error("CMP-001", definitionResult.Error!.Message, $"nodes/{node.Id}/nodeType"));
                continue;
            }

            NodeTypeDefinition definition = definitionResult.Value;
            definitions[node.Id] = definition;
            if (definition.ExecutorFactory is null)
            {
                issues.Add(Error(
                    "CMP-002",
                    $"Node type '{definition.Id}' version '{definition.Version}' has no executor factory.",
                    $"nodes/{node.Id}/executorFactory"));
            }

            ResolvedNodeParameters? resolvedNode = resolvedRecipe.FindNode(node.Id);
            if (resolvedNode is null)
            {
                issues.Add(Error(
                    "CMP-003",
                    $"Resolved Recipe does not contain parameters for node '{node.Id}'.",
                    $"nodes/{node.Id}/parameters"));
                continue;
            }

            boundParameters[node.Id] = resolvedNode.Values;
            foreach ((ParameterKey key, NodeParameterValue value) in resolvedNode.Values)
            {
                ParameterDefinition? parameter = definition.FindParameter(key);
                if (parameter is null)
                {
                    issues.Add(Error(
                        "CMP-006",
                        $"Bound parameter '{key}' is not defined by node type '{definition.Id}'.",
                        $"nodes/{node.Id}/parameters/{key}"));
                    continue;
                }

                if (!parameter.IsValid(value.Value))
                {
                    issues.Add(Error(
                        "CMP-007",
                        $"Bound parameter '{key}' is incompatible with node type '{definition.Id}'.",
                        $"nodes/{node.Id}/parameters/{key}"));
                }
            }

            foreach (ParameterDefinition required in definition.Parameters.Where(parameter => parameter.Required))
            {
                if (!resolvedNode.Values.ContainsKey(required.Key))
                {
                    issues.Add(Error(
                        "CMP-004",
                        $"Required parameter '{required.Key}' is not bound for node '{node.Id}'.",
                        $"nodes/{node.Id}/parameters/{required.Key}"));
                }
            }
        }

        if (issues.Any(issue => issue.Severity == ValidationSeverity.Error))
        {
            return new WorkflowCompilationResult(null, issues);
        }

        Dictionary<NodeId, NodeId[]> dependencies = enabledNodes.ToDictionary(
            node => node.Id,
            node => enabledConnections
                .Where(connection => connection.TargetNodeId == node.Id)
                .Select(connection => connection.SourceNodeId)
                .Distinct()
                .Order(NodeIdComparer)
                .ToArray());
        Dictionary<NodeId, NodeId[]> dependents = enabledNodes.ToDictionary(
            node => node.Id,
            node => enabledConnections
                .Where(connection => connection.SourceNodeId == node.Id)
                .Select(connection => connection.TargetNodeId)
                .Distinct()
                .Order(NodeIdComparer)
                .ToArray());

        NodeId[]? orderedNodeIds = TopologicalOrder(enabledNodes.Select(node => node.Id), dependents, dependencies);
        if (orderedNodeIds is null)
        {
            issues.Add(Error("CMP-005", "Workflow graph could not be topologically ordered.", "connections"));
            return new WorkflowCompilationResult(null, issues);
        }

        var compiledNodes = new List<CompiledNode>(orderedNodeIds.Length);
        foreach (NodeId nodeId in orderedNodeIds)
        {
            WorkflowNode workflowNode = enabledNodes.Single(node => node.Id == nodeId);
            NodeTypeDefinition definition = definitions[nodeId];
            CompiledInputBinding[] inputBindings = enabledConnections
                .Where(connection => connection.TargetNodeId == nodeId)
                .OrderBy(connection => connection.TargetPort.Value, StringComparer.Ordinal)
                .ThenBy(connection => connection.SourceNodeId.Value)
                .Select(connection => new CompiledInputBinding(
                    connection.TargetPort,
                    new PortAddress(connection.SourceNodeId, connection.SourcePort)))
                .ToArray();
            CompiledOutputBinding[] outputBindings = enabledConnections
                .Where(connection => connection.SourceNodeId == nodeId)
                .OrderBy(connection => connection.SourcePort.Value, StringComparer.Ordinal)
                .ThenBy(connection => connection.TargetNodeId.Value)
                .Select(connection => new CompiledOutputBinding(
                    connection.SourcePort,
                    new PortAddress(connection.TargetNodeId, connection.TargetPort)))
                .ToArray();
            compiledNodes.Add(new CompiledNode(
                nodeId,
                definition,
                definition.ExecutorFactory!,
                boundParameters[nodeId],
                inputBindings,
                outputBindings,
                dependencies[nodeId],
                dependents[nodeId],
                workflowNode.FailurePolicy,
                workflowNode.Timeout));
        }

        var requirements = new ExecutionResourceRequirements(
            compiledNodes.Any(node => node.ExecutionCharacteristics.UsesGpu),
            compiledNodes.Any(node => node.ExecutionCharacteristics.UsesCamera),
            compiledNodes.Any(node => node.ExecutionCharacteristics.UsesExternalIo),
            compiledNodes.Any(node => node.ExecutionCharacteristics.MayBlock));
        var plan = new ExecutionPlan(
            PlanId.New(),
            workflow.Id,
            workflow.SchemaVersion,
            resolvedRecipe.RecipeId,
            resolvedRecipe.Version,
            compiledNodes,
            orderedNodeIds.Where(nodeId => dependencies[nodeId].Length == 0),
            orderedNodeIds.Where(nodeId => dependents[nodeId].Length == 0),
            requirements,
            issues.Where(issue => issue.Severity != ValidationSeverity.Error),
            workflow.Timeout);
        return new WorkflowCompilationResult(plan, issues);
    }

    private static NodeId[]? TopologicalOrder(
        IEnumerable<NodeId> nodeIds,
        Dictionary<NodeId, NodeId[]> dependents,
        Dictionary<NodeId, NodeId[]> dependencies)
    {
        var remainingDependencies = dependencies.ToDictionary(pair => pair.Key, pair => pair.Value.Length);
        var ready = new SortedSet<NodeId>(NodeIdComparer);
        foreach (NodeId nodeId in nodeIds.Where(nodeId => remainingDependencies[nodeId] == 0))
        {
            ready.Add(nodeId);
        }

        var ordered = new List<NodeId>(remainingDependencies.Count);
        while (ready.Count > 0)
        {
            NodeId current = ready.Min!;
            ready.Remove(current);
            ordered.Add(current);
            foreach (NodeId dependent in dependents[current])
            {
                remainingDependencies[dependent]--;
                if (remainingDependencies[dependent] == 0)
                {
                    ready.Add(dependent);
                }
            }
        }

        return ordered.Count == remainingDependencies.Count ? ordered.ToArray() : null;
    }

    private static ValidationIssue Error(string code, string message, string path) =>
        new(new DomainError(code, message), ValidationSeverity.Error, path);
}
