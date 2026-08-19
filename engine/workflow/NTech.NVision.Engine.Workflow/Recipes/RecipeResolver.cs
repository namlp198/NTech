using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Workflow.Recipes;

public sealed class RecipeResolver
{
    private readonly INodeTypeRegistry registry;

    public RecipeResolver(INodeTypeRegistry registry) =>
        this.registry = registry ?? throw new ArgumentNullException(nameof(registry));

    public RecipeResolutionResult Resolve(WorkflowDefinition workflow, Recipe recipe)
    {
        ArgumentNullException.ThrowIfNull(workflow);
        ArgumentNullException.ThrowIfNull(recipe);

        var issues = new List<ValidationIssue>();
        var overridesByNode = recipe.Overrides.ToLookup(item => item.TargetNodeId);
        var resolvedByNode = new Dictionary<NodeId, ResolvedNodeParameters>();

        foreach (RecipeOverride recipeOverride in recipe.Overrides.Where(item =>
                     workflow.Nodes.All(node => node.Id != item.TargetNodeId)))
        {
            issues.Add(Error(
                "RCP-002",
                $"Override target node '{recipeOverride.TargetNodeId}' does not exist.",
                OverridePath(recipeOverride)));
        }

        foreach (WorkflowNode node in workflow.Nodes)
        {
            var definitionResult = registry.Resolve(node.NodeTypeId, node.NodeTypeVersion);
            if (definitionResult.IsFailure)
            {
                issues.Add(Error(
                    "RCP-003",
                    definitionResult.Error!.Message,
                    $"nodes/{node.Id}"));
                continue;
            }

            NodeTypeDefinition definition = definitionResult.Value;
            var effectiveValues = new Dictionary<ParameterKey, NodeParameterValue>();
            foreach (ParameterDefinition parameter in definition.Parameters.Where(item => item.DefaultValue is not null))
            {
                effectiveValues[parameter.Key] = new NodeParameterValue(
                    parameter.Key,
                    parameter.DefaultValue!,
                    ParameterValueSource.NodeDefault);
            }

            foreach (NodeParameterValue configuredValue in node.Parameters)
            {
                ApplyConfiguredValue(node, definition, configuredValue, effectiveValues, issues);
            }

            foreach (RecipeOverride recipeOverride in overridesByNode[node.Id])
            {
                ApplyOverride(node, definition, recipeOverride, effectiveValues, issues);
            }

            foreach (ParameterDefinition requiredParameter in definition.Parameters.Where(item => item.Required))
            {
                if (!effectiveValues.ContainsKey(requiredParameter.Key))
                {
                    issues.Add(Error(
                        "RCP-007",
                        $"Required parameter '{requiredParameter.Key}' has no effective value.",
                        $"nodes/{node.Id}/parameters/{requiredParameter.Key}"));
                }
            }

            resolvedByNode[node.Id] = new ResolvedNodeParameters(node.Id, effectiveValues);
        }

        bool isValid = issues.All(issue => issue.Severity != ValidationSeverity.Error);
        ResolvedRecipe? resolvedRecipe = isValid
            ? new ResolvedRecipe(recipe.Id, recipe.Version, resolvedByNode)
            : null;
        return new RecipeResolutionResult(resolvedRecipe, issues);
    }

    private static void ApplyConfiguredValue(
        WorkflowNode node,
        NodeTypeDefinition definition,
        NodeParameterValue configuredValue,
        Dictionary<ParameterKey, NodeParameterValue> effectiveValues,
        List<ValidationIssue> issues)
    {
        ParameterDefinition? parameter = definition.FindParameter(configuredValue.Key);
        if (parameter is null)
        {
            issues.Add(Error(
                "RCP-004",
                $"Parameter '{configuredValue.Key}' is not defined by node type '{definition.Id}'.",
                $"nodes/{node.Id}/parameters/{configuredValue.Key}"));
            return;
        }

        if (!ValidateValue(node.Id, parameter, configuredValue.Value, issues, "RCP-006", "RCP-008"))
        {
            return;
        }

        effectiveValues[configuredValue.Key] = new NodeParameterValue(
            configuredValue.Key,
            configuredValue.Value,
            ParameterValueSource.ProjectValue);
    }

    private static void ApplyOverride(
        WorkflowNode node,
        NodeTypeDefinition definition,
        RecipeOverride recipeOverride,
        Dictionary<ParameterKey, NodeParameterValue> effectiveValues,
        List<ValidationIssue> issues)
    {
        ParameterDefinition? parameter = definition.FindParameter(recipeOverride.ParameterKey);
        if (parameter is null)
        {
            issues.Add(Error(
                "RCP-004",
                $"Parameter '{recipeOverride.ParameterKey}' is not defined by node type '{definition.Id}'.",
                OverridePath(recipeOverride)));
            return;
        }

        if (!parameter.RecipeOverridable)
        {
            issues.Add(Error(
                "RCP-005",
                $"Parameter '{recipeOverride.ParameterKey}' is not recipe-overridable.",
                OverridePath(recipeOverride)));
            return;
        }

        if (!ValidateValue(node.Id, parameter, recipeOverride.Value, issues, "RCP-006", "RCP-008"))
        {
            return;
        }

        effectiveValues[recipeOverride.ParameterKey] = new NodeParameterValue(
            recipeOverride.ParameterKey,
            recipeOverride.Value,
            ParameterValueSource.RecipeOverride);
    }

    private static bool ValidateValue(
        NodeId nodeId,
        ParameterDefinition parameter,
        ParameterValue value,
        List<ValidationIssue> issues,
        string typeErrorCode,
        string rangeErrorCode)
    {
        string path = $"nodes/{nodeId}/parameters/{parameter.Key}";
        if (value.Kind != parameter.DataType)
        {
            issues.Add(Error(
                typeErrorCode,
                $"Parameter '{parameter.Key}' expects '{parameter.DataType}' but received '{value.Kind}'.",
                path));
            return false;
        }

        if (!parameter.IsValid(value))
        {
            issues.Add(Error(rangeErrorCode, $"Parameter '{parameter.Key}' is outside its allowed range.", path));
            return false;
        }

        return true;
    }

    private static ValidationIssue Error(string code, string message, string path) =>
        new(new DomainError(code, message), ValidationSeverity.Error, path);

    private static string OverridePath(RecipeOverride recipeOverride) =>
        $"recipe/overrides/{recipeOverride.TargetNodeId}/{recipeOverride.ParameterKey}";
}
