using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Execution;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Fakes;

public static class FakeNodeCatalog
{
    public static NodeTypeId ConstantFloatTypeId { get; } = new("nvision.test.constant-float");

    public static NodeTypeId AddFloatTypeId { get; } = new("nvision.test.add-float");

    public static NodeTypeId GreaterThanTypeId { get; } = new("nvision.test.greater-than");

    public static ParameterKey ValueParameter { get; } = new("value");

    public static ParameterKey ThresholdParameter { get; } = new("threshold");

    public static PortKey ValuePort { get; } = new("value");

    public static PortKey LeftPort { get; } = new("left");

    public static PortKey RightPort { get; } = new("right");

    public static PortKey SumPort { get; } = new("sum");

    public static PortKey DecisionPort { get; } = new("decision");

    public static IReadOnlyList<NodeTypeDefinition> CreateDefinitions() =>
    [
        CreateConstantDefinition(),
        CreateAddDefinition(),
        CreateGreaterThanDefinition(),
    ];

    private static NodeTypeDefinition CreateConstantDefinition() =>
        new(
            ConstantFloatTypeId,
            SemanticVersion.Parse("1.0.0"),
            "Fake Constant Float",
            "Testing",
            [Output(ValuePort, PortDataType.FloatingPoint)],
            [
                new ParameterDefinition(
                    ValueParameter,
                    "Value",
                    ParameterValueKind.FloatingPoint,
                    required: true,
                    recipeOverridable: true),
            ],
            new NodeExecutionCharacteristics(NodeExecutionLifetime.Stateless),
            executorFactory: new DelegateNodeExecutorFactory(() => new FakeConstantFloatExecutor()));

    private static NodeTypeDefinition CreateAddDefinition() =>
        new(
            AddFloatTypeId,
            SemanticVersion.Parse("1.0.0"),
            "Fake Add Float",
            "Testing",
            [
                Input(LeftPort, PortDataType.FloatingPoint),
                Input(RightPort, PortDataType.FloatingPoint),
                Output(SumPort, PortDataType.FloatingPoint),
            ],
            executionCharacteristics: new NodeExecutionCharacteristics(NodeExecutionLifetime.Stateless),
            executorFactory: new DelegateNodeExecutorFactory(() => new FakeAddFloatExecutor()));

    private static NodeTypeDefinition CreateGreaterThanDefinition() =>
        new(
            GreaterThanTypeId,
            SemanticVersion.Parse("1.0.0"),
            "Fake Greater Than",
            "Testing",
            [
                Input(ValuePort, PortDataType.FloatingPoint),
                Output(DecisionPort, PortDataType.Boolean),
            ],
            [
                new ParameterDefinition(
                    ThresholdParameter,
                    "Threshold",
                    ParameterValueKind.FloatingPoint,
                    ParameterValue.FromFloatingPoint(0),
                    required: true,
                    recipeOverridable: true),
            ],
            new NodeExecutionCharacteristics(NodeExecutionLifetime.Stateless),
            executorFactory: new DelegateNodeExecutorFactory(() => new FakeGreaterThanExecutor()));

    private static PortDefinition Input(PortKey key, PortDataType dataType) =>
        new(key, key.Value, PortDirection.Input, dataType, required: true);

    private static PortDefinition Output(PortKey key, PortDataType dataType) =>
        new(key, key.Value, PortDirection.Output, dataType, required: true, multiConnectionAllowed: true);

    private sealed class FakeConstantFloatExecutor : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            NodeParameterValue parameter = context.GetRequiredParameter(ValueParameter);
            if (parameter.Value.Kind != ParameterValueKind.FloatingPoint ||
                parameter.Value.FloatingPointValue is not double value)
            {
                return ValueTask.FromResult(NodeExecutionResult.Failure(
                    new DomainError("FAKE-001", "Fake constant requires a floating-point 'value' parameter.")));
            }

            return ValueTask.FromResult(NodeExecutionResult.Success(
                new Dictionary<PortKey, DataValue>
                {
                    [ValuePort] = DataValue.FromFloatingPoint(value),
                }));
        }
    }

    private sealed class FakeAddFloatExecutor : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            double left = context.GetSingleInput<double>(LeftPort, PortDataType.FloatingPoint).Value;
            double right = context.GetSingleInput<double>(RightPort, PortDataType.FloatingPoint).Value;
            return ValueTask.FromResult(NodeExecutionResult.Success(
                new Dictionary<PortKey, DataValue>
                {
                    [SumPort] = DataValue.FromFloatingPoint(left + right),
                }));
        }
    }

    private sealed class FakeGreaterThanExecutor : INodeExecutor
    {
        public ValueTask<NodeExecutionResult> ExecuteAsync(
            NodeExecutionContext context,
            CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            double value = context.GetSingleInput<double>(ValuePort, PortDataType.FloatingPoint).Value;
            NodeParameterValue threshold = context.GetRequiredParameter(ThresholdParameter);
            if (threshold.Value.Kind != ParameterValueKind.FloatingPoint ||
                threshold.Value.FloatingPointValue is not double thresholdValue)
            {
                return ValueTask.FromResult(NodeExecutionResult.Failure(
                    new DomainError("FAKE-002", "Fake comparison requires a floating-point 'threshold' parameter.")));
            }

            return ValueTask.FromResult(NodeExecutionResult.Success(
                new Dictionary<PortKey, DataValue>
                {
                    [DecisionPort] = DataValue.FromBoolean(value > thresholdValue),
                }));
        }
    }
}
