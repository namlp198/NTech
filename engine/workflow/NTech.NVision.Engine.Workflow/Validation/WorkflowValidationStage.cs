namespace NTech.NVision.Engine.Workflow.Validation;

public enum WorkflowValidationStage
{
    Schema = 100,
    Graph = 200,
    NodeType = 300,
    Port = 400,
    Parameter = 500,
    Reference = 600,
    ExecutionConstraint = 700,
}
