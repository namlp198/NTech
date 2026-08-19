using System.Text.Json;
using System.Text.Json.Serialization;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Contracts.Inspection;
using NTech.NVision.Core.Versioning;
using NTech.NVision.Domain.Parameters;
using NTech.NVision.Domain.Recipes;
using NTech.NVision.Domain.Workflows;
using NTech.NVision.Engine.Workflow.Compilation;
using NTech.NVision.Engine.Workflow.Execution;
using NTech.NVision.Engine.Workflow.Fakes;
using NTech.NVision.Engine.Workflow.Nodes;
using NTech.NVision.Engine.Workflow.Recipes;
using NTech.NVision.Engine.Workflow.Registry;
using NTech.NVision.Engine.AI.Models;
using NTech.NVision.Engine.Acquisition.Services;
using NTech.NVision.Engine.Inspection.Storage;
using NTech.NVision.Engine.Inspection.WireMarker;
using NTech.NVision.Plugin.Abstractions.Cameras;
using NTech.NVision.Plugin.AI.OnnxRuntime;
using NTech.NVision.Plugin.Camera.File;

return args.Length > 0 && string.Equals(args[0], "inspect", StringComparison.OrdinalIgnoreCase)
    ? await RunWireMarkerInspectionAsync(args[1..])
    : await RunDemoAsync();

static async Task<int> RunWireMarkerInspectionAsync(string[] arguments)
{
    if (arguments.Length != 3)
    {
        Console.Error.WriteLine("Usage: nvision-cli inspect <image.pgm> <TYPE_01|TYPE_04> <output-directory>");
        return 1;
    }

    string imagePath = Path.GetFullPath(arguments[0]);
    string configurationCode = arguments[1].Trim().ToLowerInvariant();
    string outputRoot = Path.GetFullPath(arguments[2]);
    string assets = Path.Combine(AppContext.BaseDirectory, "WireMarker");
    string configurationPath = Path.Combine(
        assets,
        "configurations",
        configurationCode.Replace("_", string.Empty, StringComparison.Ordinal) + ".json");
    if (!File.Exists(imagePath) || !File.Exists(configurationPath))
    {
        Console.Error.WriteLine("Image or Wire Marker configuration was not found.");
        return 1;
    }

    WireMarkerConfiguration? configuration = JsonSerializer.Deserialize<WireMarkerConfiguration>(
        await File.ReadAllTextAsync(configurationPath),
        new JsonSerializerOptions { PropertyNameCaseInsensitive = true });
    if (configuration is null)
    {
        Console.Error.WriteLine("Wire Marker configuration is invalid.");
        return 1;
    }

    var packageResult = await ModelPackageLoader.LoadAsync(Path.Combine(
        assets, "models", "wire-marker-1.0.0.nvmodel"));
    if (packageResult.IsFailure)
    {
        Console.Error.WriteLine($"{packageResult.Error!.Code} {packageResult.Error.Message}");
        return 1;
    }

    ModelPackage package = packageResult.Value;
    var registry = new ModelRegistry();
    registry.Register(package);
    var slot = new ModelSlot(configuration.ModelSlot);
    var resolver = new ModelSlotResolver(
        registry,
        [new ModelBinding(slot, package.Manifest.Id, package.Manifest.Version)]);
    var cameraId = new CameraId("wire-marker-file");
    await using var camera = new FileCameraDevice(cameraId, [imagePath]);
    await using var acquisition = new AcquisitionService([camera]);
    var connected = await acquisition.ConnectAsync(cameraId);
    if (connected.IsFailure)
    {
        Console.Error.WriteLine($"{connected.Error!.Code} {connected.Error.Message}");
        return 1;
    }

    Directory.CreateDirectory(outputRoot);
    var repository = new SqliteInspectionResultRepository(Path.Combine(outputRoot, "inspections.db"));
    await repository.InitializeAsync();
    using var backend = new OnnxRuntimeBackend();
    var engine = new WireMarkerInspectionEngine(
        acquisition,
        resolver,
        backend,
        repository,
        new FileImageStore(Path.Combine(outputRoot, "images")));
    var request = new WireMarkerInspectionRequest(
        cameraId,
        new FrameGrabRequest(TimeSpan.FromSeconds(2), "wire-marker"),
        slot,
        configuration.ExpectedClass,
        "WM01",
        "11111111-1111-1111-1111-111111111111",
        "1.0.0",
        "22222222-2222-2222-2222-222222222222",
        "1.0.0",
        configuration.RecipeId,
        configuration.RecipeCode,
        configuration.RecipeVersion,
        "ntech.camera.file",
        "1.0.0");
    InspectionResult result = await engine.InspectAsync(request);
    var jsonOptions = new JsonSerializerOptions
    {
        Converters = { new JsonStringEnumConverter() },
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        WriteIndented = true,
    };
    Console.WriteLine(JsonSerializer.Serialize(result, jsonOptions));
    Console.WriteLine($"SQLite: {Path.Combine(outputRoot, "inspections.db")}");
    return result.Status is InspectionStatus.Ok or InspectionStatus.Ng ? 0 : 1;
}

static async Task<int> RunDemoAsync()
{
    var registry = new NodeTypeRegistry();
    NodeTypeDefinition[] definitions = FakeNodeCatalog.CreateDefinitions().ToArray();
    foreach (NodeTypeDefinition definition in definitions)
    {
        var registration = registry.Register(definition);
        if (registration.IsFailure)
        {
            Console.Error.WriteLine($"Registry error: {registration.Error!.Code} {registration.Error.Message}");
            return 1;
        }
    }

    NodeTypeDefinition constantType = definitions.Single(item => item.Id == FakeNodeCatalog.ConstantFloatTypeId);
    NodeTypeDefinition addType = definitions.Single(item => item.Id == FakeNodeCatalog.AddFloatTypeId);
    NodeTypeDefinition decisionType = definitions.Single(item => item.Id == FakeNodeCatalog.GreaterThanTypeId);
    WorkflowNode two = Node(
        constantType,
        new NodeParameterValue(
            FakeNodeCatalog.ValueParameter,
            ParameterValue.FromFloatingPoint(2),
            ParameterValueSource.ProjectValue));
    WorkflowNode three = Node(
        constantType,
        new NodeParameterValue(
            FakeNodeCatalog.ValueParameter,
            ParameterValue.FromFloatingPoint(3),
            ParameterValueSource.ProjectValue));
    WorkflowNode add = Node(addType);
    WorkflowNode decision = Node(decisionType);
    var workflow = new WorkflowDefinition(WorkflowId.New(), "P1 Fake Deterministic Demo");
    workflow.AddNode(decision);
    workflow.AddNode(add);
    workflow.AddNode(three);
    workflow.AddNode(two);
    workflow.AddConnection(Connection(two, FakeNodeCatalog.ValuePort, add, FakeNodeCatalog.LeftPort));
    workflow.AddConnection(Connection(three, FakeNodeCatalog.ValuePort, add, FakeNodeCatalog.RightPort));
    workflow.AddConnection(Connection(add, FakeNodeCatalog.SumPort, decision, FakeNodeCatalog.ValuePort));

    var recipe = new Recipe(
        RecipeId.New(),
        "P1 Demo Recipe",
        "P1_DEMO",
        SemanticVersion.Parse("1.0.0"));
    recipe.AddOverride(new RecipeOverride(
        decision.Id,
        FakeNodeCatalog.ThresholdParameter,
        ParameterValue.FromFloatingPoint(4)));

    RecipeResolutionResult resolution = new RecipeResolver(registry).Resolve(workflow, recipe);
    if (!resolution.IsValid)
    {
        PrintIssues("Recipe resolution", resolution.Issues.Select(issue => $"{issue.Error.Code} {issue.Error.Message}"));
        return 1;
    }

    WorkflowCompilationResult compilation = new WorkflowCompiler(registry).Compile(workflow, resolution.Recipe!);
    if (!compilation.IsSuccess)
    {
        PrintIssues("Compilation", compilation.Issues.Select(issue => $"{issue.Error.Code} {issue.Error.Message}"));
        return 1;
    }

    using var runtime = new BoundedParallelWorkflowRuntime(maxConcurrency: 2);
    WorkflowExecutionResult execution = await runtime.ExecuteAsync(compilation.Plan!);
    if (!execution.IsSuccess)
    {
        Console.Error.WriteLine($"Execution error: {execution.Error?.Code} {execution.Error?.Message}");
        return 1;
    }

    var outputAddress = new PortAddress(decision.Id, FakeNodeCatalog.DecisionPort);
    var output = (TypedDataValue<bool>)execution.Outputs[outputAddress];
    Console.WriteLine("NTech NVision CLI — P1 headless Workflow demo");
    Console.WriteLine($"Workflow: {workflow.Name}");
    Console.WriteLine($"Plan nodes: {compilation.Plan!.Nodes.Count}");
    Console.WriteLine($"Maximum concurrency: {runtime.MaxConcurrency}");
    Console.WriteLine("Calculation: (2 + 3) > 4");
    Console.WriteLine($"Decision: {output.Value}");
    return 0;
}

static WorkflowNode Node(NodeTypeDefinition definition, params NodeParameterValue[] parameters) =>
    new(
        NodeId.New(),
        definition.Id,
        definition.Version,
        definition.DisplayName,
        definition.Ports,
        parameters: parameters);

static WorkflowConnection Connection(
    WorkflowNode source,
    PortKey sourcePort,
    WorkflowNode target,
    PortKey targetPort) =>
    new(ConnectionId.New(), source.Id, sourcePort, target.Id, targetPort);

static void PrintIssues(string operation, IEnumerable<string> issues)
{
    Console.Error.WriteLine($"{operation} failed:");
    foreach (string issue in issues)
    {
        Console.Error.WriteLine($"  {issue}");
    }
}

internal sealed record WireMarkerConfiguration
{
    public string RecipeId { get; init; } = string.Empty;

    public string RecipeCode { get; init; } = string.Empty;

    public string RecipeVersion { get; init; } = string.Empty;

    public string ExpectedClass { get; init; } = string.Empty;

    public string ModelSlot { get; init; } = string.Empty;
}
