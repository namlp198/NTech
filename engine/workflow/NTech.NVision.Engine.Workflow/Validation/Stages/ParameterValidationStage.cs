using NTech.NVision.Core.Validation;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Validation.Stages;

public sealed class ParameterValidationStage : IWorkflowValidationStage
{
    public WorkflowValidationStage Stage => WorkflowValidationStage.Parameter;

    public IEnumerable<ValidationIssue> Validate(WorkflowValidationContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        foreach (WorkflowNode node in context.Workflow.Nodes)
        {
            var resolution = context.Registry.Resolve(node.NodeTypeId, node.NodeTypeVersion);
            if (resolution.IsFailure)
            {
                continue;
            }

            NodeTypeDefinition definition = resolution.Value;
            foreach (NodeParameterValue configuredValue in node.Parameters)
            {
                ParameterDefinition? parameter = definition.FindParameter(configuredValue.Key);
                if (parameter is null)
                {
                    yield return Invalid(
                        node,
                        configuredValue.Key,
                        $"Parameter '{configuredValue.Key}' is not defined by node type '{definition.Id}'.");
                    continue;
                }

                if (configuredValue.Value.Kind != parameter.DataType)
                {
                    yield return Invalid(
                        node,
                        configuredValue.Key,
                        $"Parameter '{configuredValue.Key}' expects '{parameter.DataType}' but received '{configuredValue.Value.Kind}'.");
                    continue;
                }

                if (!parameter.IsValid(configuredValue.Value))
                {
                    yield return Invalid(
                        node,
                        configuredValue.Key,
                        $"Parameter '{configuredValue.Key}' is outside its allowed range.");
                }
            }

            foreach (ParameterDefinition required in definition.Parameters.Where(item => item.Required))
            {
                if (required.DefaultValue is null && node.FindParameter(required.Key) is null)
                {
                    yield return Invalid(
                        node,
                        required.Key,
                        $"Required parameter '{required.Key}' has no configured or default value.");
                }
            }
        }
    }

    private static ValidationIssue Invalid(WorkflowNode node, ParameterKey key, string message) =>
        WorkflowValidationIssueFactory.Error(
            "WFL-009",
            message,
            $"nodes/{node.Id}/parameters/{key}");
}
