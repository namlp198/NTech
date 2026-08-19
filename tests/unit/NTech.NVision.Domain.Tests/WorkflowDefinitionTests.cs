using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Domain.Tests;

public sealed class WorkflowDefinitionTests
{
    private static readonly PortKey Input = new("input");
    private static readonly PortKey Output = new("output");

    [Fact]
    public void AddConnectionBuildsTypedDirectedGraph()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode(outputType: PortDataType.Image);
        WorkflowNode target = CreateNode(inputType: PortDataType.Image);
        workflow.AddNode(source);
        workflow.AddNode(target);
        var connection = Connect(source, target);

        var result = workflow.AddConnection(connection);

        Assert.True(result.IsSuccess);
        Assert.Same(connection, Assert.Single(workflow.Connections));
    }

    [Fact]
    public void AddNodeRejectsDuplicateIdentity()
    {
        var workflow = CreateWorkflow();
        WorkflowNode node = CreateNode();
        workflow.AddNode(node);

        var result = workflow.AddNode(CreateNode(id: node.Id));

        Assert.Equal("WFL-001", result.Error?.Code);
        Assert.Single(workflow.Nodes);
    }

    [Fact]
    public void AddConnectionRejectsMissingSourceNode()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode();
        WorkflowNode target = CreateNode();
        workflow.AddNode(target);

        var result = workflow.AddConnection(Connect(source, target));

        Assert.Equal("WFL-002", result.Error?.Code);
        Assert.Empty(workflow.Connections);
    }

    [Fact]
    public void AddConnectionRejectsMissingTargetNode()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode();
        WorkflowNode target = CreateNode();
        workflow.AddNode(source);

        var result = workflow.AddConnection(Connect(source, target));

        Assert.Equal("WFL-003", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsInputUsedAsSource()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode();
        WorkflowNode target = CreateNode();
        workflow.AddNode(source);
        workflow.AddNode(target);
        var connection = new WorkflowConnection(
            ConnectionId.New(), source.Id, Input, target.Id, Input);

        var result = workflow.AddConnection(connection);

        Assert.Equal("WFL-012", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsOutputUsedAsTarget()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode();
        WorkflowNode target = CreateNode();
        workflow.AddNode(source);
        workflow.AddNode(target);
        var connection = new WorkflowConnection(
            ConnectionId.New(), source.Id, Output, target.Id, Output);

        var result = workflow.AddConnection(connection);

        Assert.Equal("WFL-013", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsIncompatiblePortTypes()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode(outputType: PortDataType.Image);
        WorkflowNode target = CreateNode(inputType: PortDataType.FloatingPoint);
        workflow.AddNode(source);
        workflow.AddNode(target);

        var result = workflow.AddConnection(Connect(source, target));

        Assert.Equal("WFL-005", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsSecondBindingToSingleInput()
    {
        var workflow = CreateWorkflow();
        WorkflowNode firstSource = CreateNode();
        WorkflowNode secondSource = CreateNode();
        WorkflowNode target = CreateNode(inputAllowsMultiple: false);
        workflow.AddNode(firstSource);
        workflow.AddNode(secondSource);
        workflow.AddNode(target);
        workflow.AddConnection(Connect(firstSource, target));

        var result = workflow.AddConnection(Connect(secondSource, target));

        Assert.Equal("WFL-014", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsSecondRouteFromSingleOutput()
    {
        var workflow = CreateWorkflow();
        WorkflowNode source = CreateNode(outputAllowsMultiple: false);
        WorkflowNode firstTarget = CreateNode();
        WorkflowNode secondTarget = CreateNode();
        workflow.AddNode(source);
        workflow.AddNode(firstTarget);
        workflow.AddNode(secondTarget);
        workflow.AddConnection(Connect(source, firstTarget));

        var result = workflow.AddConnection(Connect(source, secondTarget));

        Assert.Equal("WFL-014", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsSelfConnection()
    {
        var workflow = CreateWorkflow();
        WorkflowNode node = CreateNode();
        workflow.AddNode(node);

        var result = workflow.AddConnection(Connect(node, node));

        Assert.Equal("WFL-015", result.Error?.Code);
    }

    [Fact]
    public void AddConnectionRejectsCycle()
    {
        var workflow = CreateWorkflow();
        WorkflowNode first = CreateNode();
        WorkflowNode second = CreateNode();
        WorkflowNode third = CreateNode();
        workflow.AddNode(first);
        workflow.AddNode(second);
        workflow.AddNode(third);
        workflow.AddConnection(Connect(first, second));
        workflow.AddConnection(Connect(second, third));

        var result = workflow.AddConnection(Connect(third, first));

        Assert.Equal("WFL-004", result.Error?.Code);
        Assert.Equal(2, workflow.Connections.Count);
    }

    [Fact]
    public void NodeRejectsDuplicatePortKeyWithinDirection()
    {
        PortDefinition[] ports =
        [
            Port(Input, PortDirection.Input, PortDataType.Image),
            Port(Input, PortDirection.Input, PortDataType.Image),
        ];

        Assert.Throws<ArgumentException>(() =>
            new WorkflowNode(NodeId.New(), new NodeTypeId("nvision.test.node"), "1.0.0", "Node", ports));
    }

    [Fact]
    public void PortDataTypeSupportsBuiltInAndExtensionIdentifiers()
    {
        var extension = new PortDataType("vendor.data.thermal-image");

        Assert.Equal("nvision.data.image", PortDataType.Image.Value);
        Assert.Equal("vendor.data.thermal-image", extension.Value);
    }

    [Fact]
    public void WorkflowCollectionsCannotBeMutatedThroughPublicApi()
    {
        var workflow = CreateWorkflow();
        var collection = Assert.IsAssignableFrom<ICollection<WorkflowNode>>(workflow.Nodes);

        Assert.Throws<NotSupportedException>(() => collection.Add(CreateNode()));
    }

    [Fact]
    public void WorkflowAndNodeTimeoutsMustBePositiveWhenConfigured()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new WorkflowDefinition(WorkflowId.New(), "Inspection", timeout: TimeSpan.Zero));

        PortDefinition[] ports =
        [
            Port(Input, PortDirection.Input, PortDataType.Image),
            Port(Output, PortDirection.Output, PortDataType.Image),
        ];
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new WorkflowNode(
                NodeId.New(),
                new NodeTypeId("nvision.test.passthrough"),
                "1.0.0",
                "Pass Through",
                ports,
                timeout: TimeSpan.FromMilliseconds(-1)));
    }

    private static WorkflowDefinition CreateWorkflow() => new(WorkflowId.New(), "Inspection");

    private static WorkflowConnection Connect(WorkflowNode source, WorkflowNode target) =>
        new(ConnectionId.New(), source.Id, Output, target.Id, Input);

    private static WorkflowNode CreateNode(
        NodeId? id = null,
        PortDataType? inputType = null,
        PortDataType? outputType = null,
        bool inputAllowsMultiple = false,
        bool outputAllowsMultiple = true)
    {
        PortDefinition[] ports =
        [
            Port(Input, PortDirection.Input, inputType ?? PortDataType.Image, inputAllowsMultiple),
            Port(Output, PortDirection.Output, outputType ?? PortDataType.Image, outputAllowsMultiple),
        ];

        return new WorkflowNode(
            id ?? NodeId.New(),
            new NodeTypeId("nvision.test.passthrough"),
            "1.0.0",
            "Pass Through",
            ports);
    }

    private static PortDefinition Port(
        PortKey key,
        PortDirection direction,
        PortDataType dataType,
        bool multiConnectionAllowed = false) =>
        new(key, key.Value, direction, dataType, multiConnectionAllowed: multiConnectionAllowed);
}
