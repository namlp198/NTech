using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Workflow.Tests;

internal static class TestNodeTypes
{
    public static readonly NodeTypeId DefaultId = new("nvision.test.parameterized");
    public static readonly ParameterKey Threshold = new("threshold");

    public static NodeTypeDefinition CreateDefinition(
        NodeTypeId? id = null,
        IEnumerable<PortDefinition>? ports = null,
        IEnumerable<ParameterDefinition>? parameters = null,
        NodeExecutionCharacteristics? characteristics = null,
        NodeTypeOrigin? origin = null) =>
        new(
            id ?? DefaultId,
            SemanticVersion.Parse("1.0.0"),
            "Parameterized Test Node",
            "Test",
            ports ?? [],
            parameters ?? DefaultParameters(),
            characteristics,
            origin);

    public static WorkflowNode CreateNode(
        NodeTypeDefinition? definition = null,
        NodeId? id = null,
        bool enabled = true,
        IEnumerable<NodeParameterValue>? parameters = null,
        IEnumerable<PortDefinition>? ports = null,
        NodeFailurePolicy failurePolicy = NodeFailurePolicy.AbortWorkflow,
        TimeSpan? timeout = null)
    {
        definition ??= CreateDefinition();
        return new WorkflowNode(
            id ?? NodeId.New(),
            definition.Id,
            definition.Version,
            definition.DisplayName,
            ports ?? definition.Ports,
            enabled,
            parameters,
            failurePolicy,
            timeout);
    }

    public static WorkflowDefinition CreateWorkflow(params WorkflowNode[] nodes)
    {
        var workflow = new WorkflowDefinition(WorkflowId.New(), "Inspection");
        foreach (WorkflowNode node in nodes)
        {
            workflow.AddNode(node);
        }

        return workflow;
    }

    public static NodeTypeRegistry CreateRegistry(params NodeTypeDefinition[] definitions)
    {
        var registry = new NodeTypeRegistry();
        foreach (NodeTypeDefinition definition in definitions.Length == 0 ? [CreateDefinition()] : definitions)
        {
            registry.Register(definition);
        }

        return registry;
    }

    private static ParameterDefinition[] DefaultParameters() =>
    [
        new(
            Threshold,
            "Threshold",
            ParameterValueKind.FloatingPoint,
            ParameterValue.FromFloatingPoint(0.5),
            required: true,
            minimum: 0,
            maximum: 1,
            recipeOverridable: true),
        new(
            new ParameterKey("locked"),
            "Locked",
            ParameterValueKind.FloatingPoint,
            ParameterValue.FromFloatingPoint(0.5)),
    ];
}
