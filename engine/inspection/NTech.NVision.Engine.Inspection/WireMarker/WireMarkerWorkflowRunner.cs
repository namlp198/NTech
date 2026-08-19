using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Contracts.Inspection;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.AI.Inference;
using NTech.NVision.Engine.AI.Models;
using NTech.NVision.Engine.AI.Postprocessing;
using NTech.NVision.Engine.Inspection.Nodes;
using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Engine.Workflow.Execution;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Recipes;
using NTech.NVision.Engine.Workflow.Registry;

namespace NTech.NVision.Engine.Inspection.WireMarker;

public sealed record WireMarkerWorkflowOutcome(
    ClassificationPrediction Prediction,
    InspectionStatus Status,
    string DecisionCode,
    TimeSpan CropDuration,
    TimeSpan ClassificationDuration,
    TimeSpan DecisionDuration);

public static class WireMarkerWorkflowRunner
{
    private static readonly SemanticVersion Version = SemanticVersion.Parse("1.0.0");
    private static readonly PortKey ImagePort = new("image");
    private static readonly PortKey ClassificationPort = new("classification");
    private static readonly PortKey MatchPort = new("match");
    private static readonly PortKey DecisionPort = new("decision");

    public static async ValueTask<WireMarkerWorkflowOutcome> ExecuteAsync(
        Frame frame,
        ModelPackage package,
        IAiInferenceBackend backend,
        string expectedClass,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(frame);
        ArgumentNullException.ThrowIfNull(package);
        ArgumentNullException.ThrowIfNull(backend);
        ArgumentException.ThrowIfNullOrWhiteSpace(expectedClass);
        FrameLease? croppedLease = null;
        NodeTypeDefinition[] definitions = CreateDefinitions(
            frame,
            lease => croppedLease = lease,
            package,
            backend,
            expectedClass);
        var registry = new NodeTypeRegistry();
        foreach (NodeTypeDefinition definition in definitions)
        {
            Ensure(registry.Register(definition).IsSuccess, "Wire Marker node registration failed.");
        }

        WorkflowNode source = Node(definitions[0], "Source");
        WorkflowNode crop = Node(definitions[1], "Crop");
        WorkflowNode classify = Node(definitions[2], "Classify");
        WorkflowNode compare = Node(definitions[3], "Compare");
        WorkflowNode decision = Node(definitions[4], "Decision");
        var workflow = new WorkflowDefinition(WorkflowId.New(), "Wire Marker Headless Inspection");
        foreach (WorkflowNode node in new[] { source, crop, classify, compare, decision })
        {
            Ensure(workflow.AddNode(node).IsSuccess, "Wire Marker node insertion failed.");
        }

        Connect(workflow, source, ImagePort, crop, ImagePort);
        Connect(workflow, crop, ImagePort, classify, ImagePort);
        Connect(workflow, classify, ClassificationPort, compare, ClassificationPort);
        Connect(workflow, compare, MatchPort, decision, MatchPort);
        Connect(workflow, classify, ClassificationPort, decision, ClassificationPort);
        var recipe = new Recipe(RecipeId.New(), "Wire Marker Runtime Binding", "WIRE_MARKER_RUNTIME", Version);
        RecipeResolutionResult resolution = new RecipeResolver(registry).Resolve(workflow, recipe);
        Ensure(resolution.IsValid, "Wire Marker recipe resolution failed.");
        WorkflowCompilationResult compilation = new WorkflowCompiler(registry).Compile(workflow, resolution.Recipe!);
        Ensure(compilation.IsSuccess, "Wire Marker workflow compilation failed.");

        try
        {
            WorkflowExecutionResult execution = await new SequentialWorkflowRuntime()
                .ExecuteAsync(compilation.Plan!, cancellationToken).ConfigureAwait(false);
            Ensure(execution.IsSuccess, execution.Error?.Message ?? "Wire Marker workflow execution failed.");
            WireMarkerDecision finalDecision = execution.Outputs[
                new PortAddress(decision.Id, DecisionPort)] is TypedDataValue<WireMarkerDecision> decisionValue
                    ? decisionValue.Value
                    : throw new InvalidOperationException("Wire Marker decision output is missing.");
            return new WireMarkerWorkflowOutcome(
                finalDecision.Prediction,
                finalDecision.Status,
                finalDecision.Code,
                Duration(execution, crop),
                Duration(execution, classify),
                Duration(execution, compare) + Duration(execution, decision));
        }
        finally
        {
            croppedLease?.Dispose();
        }
    }

    private static NodeTypeDefinition[] CreateDefinitions(
        Frame sourceFrame,
        Action<FrameLease> captureCrop,
        ModelPackage package,
        IAiInferenceBackend backend,
        string expectedClass) =>
    [
        Definition("source", "Frame Source", [], [Output(ImagePort, PortDataType.Image)],
            new SourceExecutor(sourceFrame)),
        Definition("crop", "Crop", [Input(ImagePort, PortDataType.Image)], [Output(ImagePort, PortDataType.Image)],
            new CropExecutor(captureCrop)),
        Definition("classify", "Classify", [Input(ImagePort, PortDataType.Image)],
            [Output(ClassificationPort, PortDataType.Classification)], new ClassifyExecutor(package, backend)),
        Definition("compare", "Compare", [Input(ClassificationPort, PortDataType.Classification)],
            [Output(MatchPort, PortDataType.Boolean)], new CompareExecutor(expectedClass)),
        Definition("decision", "Decision", [
            Input(MatchPort, PortDataType.Boolean),
            Input(ClassificationPort, PortDataType.Classification),
        ],
            [Output(DecisionPort, PortDataType.Decision)], new DecisionExecutor()),
    ];

    private static NodeTypeDefinition Definition(
        string suffix,
        string name,
        IReadOnlyList<PortDefinition> inputs,
        IReadOnlyList<PortDefinition> outputs,
        INodeExecutor executor) =>
        new(
            new NodeTypeId($"nvision.wire-marker.{suffix}"),
            Version,
            name,
            "Wire Marker",
            [.. inputs, .. outputs],
            executionCharacteristics: new NodeExecutionCharacteristics(NodeExecutionLifetime.PerExecution),
            executorFactory: new DelegateNodeExecutorFactory(() => executor));

    private static WorkflowNode Node(NodeTypeDefinition definition, string name) => new(
        NodeId.New(), definition.Id, definition.Version, name, definition.Ports);

    private static PortDefinition Input(PortKey key, PortDataType type) =>
        new(key, key.Value, PortDirection.Input, type, required: true);

    private static PortDefinition Output(PortKey key, PortDataType type) =>
        new(key, key.Value, PortDirection.Output, type, required: true, multiConnectionAllowed: true);

    private static void Connect(
        WorkflowDefinition workflow,
        WorkflowNode source,
        PortKey sourcePort,
        WorkflowNode target,
        PortKey targetPort) =>
        Ensure(workflow.AddConnection(new WorkflowConnection(
            ConnectionId.New(), source.Id, sourcePort, target.Id, targetPort)).IsSuccess,
            "Wire Marker connection failed.");

    private static TimeSpan Duration(WorkflowExecutionResult execution, WorkflowNode node) =>
        execution.NodeResults.Single(result => result.NodeId == node.Id).Timing?.Duration ?? TimeSpan.Zero;

    private static void Ensure(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }

    private sealed record WireMarkerDecision(
        InspectionStatus Status,
        string Code,
        ClassificationPrediction Prediction);

    private sealed class SourceExecutor(Frame frame) : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return ValueTask.FromResult(NodeExecutionResult.Success(new Dictionary<PortKey, DataValue>
            {
                [ImagePort] = new TypedDataValue<Frame>(PortDataType.Image, frame),
            }));
        }
    }

    private sealed class CropExecutor(Action<FrameLease> capture) : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            Frame source = context.GetSingleInput<Frame>(ImagePort, PortDataType.Image).Value;
            FrameLease lease = CropNode.Execute(source, 0, 0, source.Width, source.Height);
            capture(lease);
            return ValueTask.FromResult(NodeExecutionResult.Success(new Dictionary<PortKey, DataValue>
            {
                [ImagePort] = new TypedDataValue<FrameLease>(PortDataType.Image, lease),
            }));
        }
    }

    private sealed class ClassifyExecutor(ModelPackage package, IAiInferenceBackend backend) : INodeExecutor
    {
        public async ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            FrameLease lease = context.GetSingleInput<FrameLease>(ImagePort, PortDataType.Image).Value;
            ClassificationPrediction prediction = await ClassifyNode.ExecuteAsync(
                lease.Frame, package, backend, cancellationToken).ConfigureAwait(false);
            return NodeExecutionResult.Success(new Dictionary<PortKey, DataValue>
            {
                [ClassificationPort] = new TypedDataValue<ClassificationPrediction>(
                    PortDataType.Classification, prediction),
            });
        }
    }

    private sealed class CompareExecutor(string expectedClass) : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            ClassificationPrediction prediction = context.GetSingleInput<ClassificationPrediction>(
                ClassificationPort, PortDataType.Classification).Value;
            return ValueTask.FromResult(NodeExecutionResult.Success(new Dictionary<PortKey, DataValue>
            {
                [MatchPort] = DataValue.FromBoolean(CompareNode.Execute(prediction, expectedClass)),
            }));
        }
    }

    private sealed class DecisionExecutor : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            bool accepted = context.GetSingleInput<bool>(MatchPort, PortDataType.Boolean).Value;
            ClassificationPrediction prediction = context.GetSingleInput<ClassificationPrediction>(
                ClassificationPort, PortDataType.Classification).Value;
            (InspectionStatus status, string code) = DecisionNode.Execute(accepted);
            return ValueTask.FromResult(NodeExecutionResult.Success(new Dictionary<PortKey, DataValue>
            {
                [DecisionPort] = new TypedDataValue<WireMarkerDecision>(
                    PortDataType.Decision, new WireMarkerDecision(status, code, prediction)),
            }));
        }
    }
}
