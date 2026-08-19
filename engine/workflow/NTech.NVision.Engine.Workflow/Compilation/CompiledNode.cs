using System.Collections.ObjectModel;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Execution;
using NTech.NVision.Engine.Workflow.Nodes;

namespace NTech.NVision.Engine.Workflow.Compilation;

public sealed class CompiledNode
{
    private readonly ReadOnlyDictionary<ParameterKey, NodeParameterValue> boundParameters;
    private readonly ReadOnlyCollection<NodeId> dependencies;
    private readonly ReadOnlyCollection<NodeId> dependents;
    private readonly ReadOnlyCollection<CompiledInputBinding> inputBindings;
    private readonly ReadOnlyCollection<CompiledOutputBinding> outputBindings;

    internal CompiledNode(
        NodeId nodeId,
        NodeTypeDefinition nodeType,
        INodeExecutorFactory executorFactory,
        IReadOnlyDictionary<ParameterKey, NodeParameterValue> boundParameters,
        IEnumerable<CompiledInputBinding> inputBindings,
        IEnumerable<CompiledOutputBinding> outputBindings,
        IEnumerable<NodeId> dependencies,
        IEnumerable<NodeId> dependents,
        NodeFailurePolicy failurePolicy,
        TimeSpan? timeout)
    {
        NodeId = nodeId ?? throw new ArgumentNullException(nameof(nodeId));
        NodeType = nodeType ?? throw new ArgumentNullException(nameof(nodeType));
        ExecutorFactory = executorFactory ?? throw new ArgumentNullException(nameof(executorFactory));
        ArgumentNullException.ThrowIfNull(boundParameters);
        this.boundParameters = new ReadOnlyDictionary<ParameterKey, NodeParameterValue>(
            new Dictionary<ParameterKey, NodeParameterValue>(boundParameters));
        this.inputBindings = Array.AsReadOnly(inputBindings?.ToArray() ?? throw new ArgumentNullException(nameof(inputBindings)));
        this.outputBindings = Array.AsReadOnly(outputBindings?.ToArray() ?? throw new ArgumentNullException(nameof(outputBindings)));
        this.dependencies = Array.AsReadOnly(dependencies?.ToArray() ?? throw new ArgumentNullException(nameof(dependencies)));
        this.dependents = Array.AsReadOnly(dependents?.ToArray() ?? throw new ArgumentNullException(nameof(dependents)));
        FailurePolicy = failurePolicy;
        Timeout = timeout;
    }

    public NodeId NodeId { get; }

    public NodeTypeDefinition NodeType { get; }

    public INodeExecutorFactory ExecutorFactory { get; }

    public IReadOnlyDictionary<ParameterKey, NodeParameterValue> BoundParameters => boundParameters;

    public IReadOnlyList<CompiledInputBinding> InputBindings => inputBindings;

    public IReadOnlyList<CompiledOutputBinding> OutputBindings => outputBindings;

    public IReadOnlyList<NodeId> Dependencies => dependencies;

    public IReadOnlyList<NodeId> Dependents => dependents;

    public NodeFailurePolicy FailurePolicy { get; }

    public TimeSpan? Timeout { get; }

    public NodeExecutionCharacteristics ExecutionCharacteristics => NodeType.ExecutionCharacteristics;
}
