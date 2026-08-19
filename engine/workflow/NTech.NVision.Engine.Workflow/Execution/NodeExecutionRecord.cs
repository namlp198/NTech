using NTech.NVision.Domain.Workflows;

namespace NTech.NVision.Engine.Workflow.Execution;

public sealed record NodeExecutionRecord
{
    public NodeExecutionRecord(
        NodeId nodeId,
        NodeExecutionResult result,
        NodeExecutionTiming? timing = null)
    {
        NodeId = nodeId ?? throw new ArgumentNullException(nameof(nodeId));
        Result = result ?? throw new ArgumentNullException(nameof(result));
        Timing = timing;
    }

    public NodeId NodeId { get; }

    public NodeExecutionResult Result { get; }

    public NodeExecutionTiming? Timing { get; }
}
