using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Engine.Workflow.Fakes;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Recipes;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Workflow.Tests;

internal sealed class FakeWorkflowFixture
{
    private FakeWorkflowFixture(
        NodeTypeRegistry registry,
        WorkflowDefinition workflow,
        Recipe recipe,
        WorkflowNode firstConstant,
        WorkflowNode secondConstant,
        WorkflowNode add,
        WorkflowNode decision)
    {
        Registry = registry;
        Workflow = workflow;
        Recipe = recipe;
        FirstConstant = firstConstant;
        SecondConstant = secondConstant;
        Add = add;
        Decision = decision;
    }

    public NodeTypeRegistry Registry { get; }

    public WorkflowDefinition Workflow { get; }

    public Recipe Recipe { get; }

    public WorkflowNode FirstConstant { get; }

    public WorkflowNode SecondConstant { get; }

    public WorkflowNode Add { get; }

    public WorkflowNode Decision { get; }

    public static FakeWorkflowFixture Create()
    {
        var registry = new NodeTypeRegistry();
        NodeTypeDefinition[] definitions = FakeNodeCatalog.CreateDefinitions().ToArray();
        foreach (NodeTypeDefinition definition in definitions)
        {
            registry.Register(definition);
        }

        NodeTypeDefinition constantType = definitions.Single(item => item.Id == FakeNodeCatalog.ConstantFloatTypeId);
        NodeTypeDefinition addType = definitions.Single(item => item.Id == FakeNodeCatalog.AddFloatTypeId);
        NodeTypeDefinition decisionType = definitions.Single(item => item.Id == FakeNodeCatalog.GreaterThanTypeId);
        WorkflowNode firstConstant = Node(
            constantType,
            new NodeParameterValue(
                FakeNodeCatalog.ValueParameter,
                ParameterValue.FromFloatingPoint(2),
                ParameterValueSource.ProjectValue));
        WorkflowNode secondConstant = Node(
            constantType,
            new NodeParameterValue(
                FakeNodeCatalog.ValueParameter,
                ParameterValue.FromFloatingPoint(3),
                ParameterValueSource.ProjectValue));
        WorkflowNode add = Node(addType);
        WorkflowNode decision = Node(decisionType);
        var workflow = new WorkflowDefinition(WorkflowId.New(), "Deterministic Fake Workflow");

        // Deliberately add nodes out of dependency order; the compiler owns scheduling order.
        workflow.AddNode(decision);
        workflow.AddNode(add);
        workflow.AddNode(secondConstant);
        workflow.AddNode(firstConstant);
        workflow.AddConnection(Connection(firstConstant, FakeNodeCatalog.ValuePort, add, FakeNodeCatalog.LeftPort));
        workflow.AddConnection(Connection(secondConstant, FakeNodeCatalog.ValuePort, add, FakeNodeCatalog.RightPort));
        workflow.AddConnection(Connection(add, FakeNodeCatalog.SumPort, decision, FakeNodeCatalog.ValuePort));

        var recipe = new Recipe(
            RecipeId.New(),
            "Fake Recipe",
            "FAKE",
            SemanticVersion.Parse("1.0.0"));
        recipe.AddOverride(new RecipeOverride(
            decision.Id,
            FakeNodeCatalog.ThresholdParameter,
            ParameterValue.FromFloatingPoint(4)));
        return new FakeWorkflowFixture(
            registry,
            workflow,
            recipe,
            firstConstant,
            secondConstant,
            add,
            decision);
    }

    public ExecutionPlan Compile()
    {
        RecipeResolutionResult resolution = new RecipeResolver(Registry).Resolve(Workflow, Recipe);
        Assert.True(resolution.IsValid);
        WorkflowCompilationResult compilation = new WorkflowCompiler(Registry)
            .Compile(Workflow, resolution.Recipe!);
        Assert.True(compilation.IsSuccess);
        return compilation.Plan!;
    }

    private static WorkflowNode Node(
        NodeTypeDefinition definition,
        params NodeParameterValue[] parameters) =>
        new(
            NodeId.New(),
            definition.Id,
            definition.Version,
            definition.DisplayName,
            definition.Ports,
            parameters: parameters);

    private static WorkflowConnection Connection(
        WorkflowNode source,
        PortKey sourcePort,
        WorkflowNode target,
        PortKey targetPort) =>
        new(ConnectionId.New(), source.Id, sourcePort, target.Id, targetPort);
}
