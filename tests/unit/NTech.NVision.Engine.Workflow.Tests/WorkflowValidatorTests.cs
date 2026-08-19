using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Registry;
using NTech.NVision.Engine.Workflow.Validation;

namespace NTech.NVision.Engine.Workflow.Tests;

public sealed class WorkflowValidatorTests
{
    [Fact]
    public void DefaultPipelineUsesDocumentedStageOrder()
    {
        var validator = new WorkflowValidator(new NodeTypeRegistry());

        WorkflowValidationStage[] stages = validator.Stages.Select(stage => stage.Stage).ToArray();

        Assert.Equal(
            [
                WorkflowValidationStage.Schema,
                WorkflowValidationStage.Graph,
                WorkflowValidationStage.NodeType,
                WorkflowValidationStage.Port,
                WorkflowValidationStage.Parameter,
                WorkflowValidationStage.Reference,
                WorkflowValidationStage.ExecutionConstraint,
            ],
            stages);
    }

    [Fact]
    public void ValidTypedWorkflowPassesAllStages()
    {
        PortDefinition output = Port("image", PortDirection.Output, required: false, allowsMultiple: true);
        PortDefinition input = Port("image", PortDirection.Input, required: true);
        NodeTypeDefinition sourceType = TestNodeTypes.CreateDefinition(
            new NodeTypeId("nvision.test.source"),
            [output],
            []);
        NodeTypeDefinition targetType = TestNodeTypes.CreateDefinition(
            new NodeTypeId("nvision.test.target"),
            [input],
            []);
        WorkflowNode source = TestNodeTypes.CreateNode(sourceType);
        WorkflowNode target = TestNodeTypes.CreateNode(targetType);
        WorkflowDefinition workflow = TestNodeTypes.CreateWorkflow(source, target);
        workflow.AddConnection(new WorkflowConnection(
            ConnectionId.New(), source.Id, output.Key, target.Id, input.Key));

        WorkflowValidationResult result = new WorkflowValidator(
            TestNodeTypes.CreateRegistry(sourceType, targetType)).Validate(workflow);

        Assert.True(result.IsValid);
        Assert.Empty(result.Diagnostics);
    }

    [Fact]
    public void UnknownNodeTypeIsReportedByNodeTypeStage()
    {
        WorkflowNode node = TestNodeTypes.CreateNode();

        WorkflowValidationResult result = new WorkflowValidator(new NodeTypeRegistry())
            .Validate(TestNodeTypes.CreateWorkflow(node));

        WorkflowValidationDiagnostic diagnostic = Assert.Single(result.Diagnostics);
        Assert.Equal(WorkflowValidationStage.NodeType, diagnostic.Stage);
        Assert.Equal("WFL-007", diagnostic.Issue.Error.Code);
    }

    [Fact]
    public void UnsupportedNodeVersionIsRejectedWithoutFallback()
    {
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition();
        var node = new WorkflowNode(
            NodeId.New(),
            definition.Id,
            SemanticVersion.Parse("2.0.0"),
            "Newer Node",
            definition.Ports);

        WorkflowValidationResult result = new WorkflowValidator(TestNodeTypes.CreateRegistry(definition))
            .Validate(TestNodeTypes.CreateWorkflow(node));

        Assert.Contains(result.Errors, issue => issue.Error.Code == "WFL-008");
    }

    [Fact]
    public void MissingRequiredInputIsReportedByPortStage()
    {
        PortDefinition input = Port("image", PortDirection.Input, required: true);
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(ports: [input], parameters: []);
        WorkflowNode node = TestNodeTypes.CreateNode(definition);

        WorkflowValidationResult result = new WorkflowValidator(TestNodeTypes.CreateRegistry(definition))
            .Validate(TestNodeTypes.CreateWorkflow(node));

        Assert.Contains(
            result.Diagnostics,
            diagnostic => diagnostic.Stage == WorkflowValidationStage.Port && diagnostic.Issue.Error.Code == "WFL-006");
    }

    [Fact]
    public void NodePortSnapshotMustMatchRegisteredContract()
    {
        PortDefinition registered = Port("value", PortDirection.Output, dataType: PortDataType.Image);
        PortDefinition staleSnapshot = Port("value", PortDirection.Output, dataType: PortDataType.FloatingPoint);
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(ports: [registered], parameters: []);
        WorkflowNode node = TestNodeTypes.CreateNode(definition, ports: [staleSnapshot]);

        WorkflowValidationResult result = new WorkflowValidator(TestNodeTypes.CreateRegistry(definition))
            .Validate(TestNodeTypes.CreateWorkflow(node));

        Assert.Contains(result.Errors, issue => issue.Error.Code == "WFL-018");
    }

    [Fact]
    public void InvalidParameterRangeIsReportedByParameterStage()
    {
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition();
        WorkflowNode node = TestNodeTypes.CreateNode(
            definition,
            parameters:
            [
                new(
                    TestNodeTypes.Threshold,
                    ParameterValue.FromFloatingPoint(1.5),
                    ParameterValueSource.ProjectValue),
            ]);

        WorkflowValidationResult result = new WorkflowValidator(TestNodeTypes.CreateRegistry(definition))
            .Validate(TestNodeTypes.CreateWorkflow(node));

        Assert.Contains(
            result.Diagnostics,
            diagnostic => diagnostic.Stage == WorkflowValidationStage.Parameter && diagnostic.Issue.Error.Code == "WFL-009");
    }

    [Fact]
    public void BlockingNodeWithoutCancellationProducesWarningNotError()
    {
        var characteristics = new NodeExecutionCharacteristics(
            NodeExecutionLifetime.PerExecution,
            MayBlock: true,
            SupportsCancellation: false);
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(
            parameters: [],
            characteristics: characteristics);
        WorkflowNode node = TestNodeTypes.CreateNode(definition);

        WorkflowValidationResult result = new WorkflowValidator(TestNodeTypes.CreateRegistry(definition))
            .Validate(TestNodeTypes.CreateWorkflow(node));

        Assert.True(result.IsValid);
        Assert.Contains(result.Warnings, issue => issue.Error.Code == "WFL-030");
    }

    [Fact]
    public void NodeTimeoutCannotExceedWorkflowTimeout()
    {
        NodeTypeDefinition definition = TestNodeTypes.CreateDefinition(parameters: []);
        WorkflowNode node = TestNodeTypes.CreateNode(
            definition,
            timeout: TimeSpan.FromSeconds(2));
        var workflow = new WorkflowDefinition(
            WorkflowId.New(),
            "Timeout validation",
            timeout: TimeSpan.FromSeconds(1));
        workflow.AddNode(node);

        WorkflowValidationResult result = new WorkflowValidator(TestNodeTypes.CreateRegistry(definition))
            .Validate(workflow);

        Assert.Contains(
            result.Diagnostics,
            diagnostic =>
                diagnostic.Stage == WorkflowValidationStage.ExecutionConstraint &&
                diagnostic.Issue.Error.Code == "WFL-031");
    }

    [Fact]
    public void UnsupportedSchemaAndEmptyGraphAreAttributedToSeparateStages()
    {
        var workflow = new WorkflowDefinition(WorkflowId.New(), "Invalid", "2.0");

        WorkflowValidationResult result = new WorkflowValidator(new NodeTypeRegistry()).Validate(workflow);

        Assert.Contains(
            result.Diagnostics,
            diagnostic => diagnostic.Stage == WorkflowValidationStage.Schema && diagnostic.Issue.Error.Code == "WFL-020");
        Assert.Contains(
            result.Diagnostics,
            diagnostic => diagnostic.Stage == WorkflowValidationStage.Graph && diagnostic.Issue.Error.Code == "WFL-021");
    }

    private static PortDefinition Port(
        string key,
        PortDirection direction,
        bool required = false,
        bool allowsMultiple = false,
        PortDataType? dataType = null) =>
        new(
            new PortKey(key),
            key,
            direction,
            dataType ?? PortDataType.Image,
            required,
            allowsMultiple);
}
