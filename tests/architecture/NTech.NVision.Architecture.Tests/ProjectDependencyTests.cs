using System.Xml.Linq;

namespace NTech.NVision.Architecture.Tests;

public sealed class ProjectDependencyTests
{
    [Fact]
    public void RequiredP0ProjectsArePresent()
    {
        RepositoryModel model = RepositoryModel.Load();
        string[] requiredProjects =
        [
            "NTech.NVision.Core",
            "NTech.NVision.Domain",
            "NTech.NVision.Contracts",
            "NTech.NVision.Application",
            "NTech.NVision.Engine.Workflow",
            "NTech.NVision.Engine.Acquisition",
            "NTech.NVision.Engine.AI",
            "NTech.NVision.Engine.Inspection",
            "NTech.NVision.Plugin.Abstractions",
            "NTech.NVision.Native",
            "NTech.NVision.Cli",
            "NTech.NVision.Architecture.Tests",
        ];

        foreach (string projectName in requiredProjects)
        {
            Assert.True(model.ProjectsByName.ContainsKey(projectName), $"Missing required P0 project: {projectName}");
        }
    }

    [Fact]
    public void NativeFirstFoundationAndStableCAbiArePresent()
    {
        string root = RepositoryModel.Load().RootPath;
        string[] requiredFiles =
        [
            "CMakeLists.txt",
            "CMakePresets.json",
            "native/CMakeLists.txt",
            "native/core/include/nvision/core/engine.hpp",
            "native/abi/include/nvision/capi/nvision.h",
            "native/host/proto/nvision_engine_host.proto",
            "docs/decisions/ADR-0005-native-first-core-and-engine.md",
        ];

        Assert.All(requiredFiles, path => Assert.True(
            File.Exists(Path.Combine(root, path.Replace('/', Path.DirectorySeparatorChar))),
            $"Missing native-first foundation file: {path}"));

        string abi = File.ReadAllText(Path.Combine(
            root,
            "native",
            "abi",
            "include",
            "nvision",
            "capi",
            "nvision.h"));
        Assert.Contains("NVISION_ABI_VERSION", abi, StringComparison.Ordinal);
        Assert.Contains("typedef struct nv_engine nv_engine", abi, StringComparison.Ordinal);
    }

    [Fact]
    public void ProjectGraphIsAcyclic()
    {
        RepositoryModel model = RepositoryModel.Load();
        string? cycle = model.FindCycle();

        Assert.True(cycle is null, $"Circular project dependency detected: {cycle}");
    }

    [Fact]
    public void CoreHasNoProjectDependencies()
    {
        ProjectNode core = RepositoryModel.Load().GetProject("NTech.NVision.Core");

        Assert.Empty(core.ProjectReferences);
    }

    [Theory]
    [InlineData("NTech.NVision.Domain")]
    [InlineData("NTech.NVision.Contracts")]
    public void FoundationalProjectsDependOnlyOnCore(string projectName)
    {
        RepositoryModel model = RepositoryModel.Load();
        ProjectNode project = model.GetProject(projectName);
        string[] dependencies = project.ProjectReferences
            .Select(model.GetProjectByPath)
            .Select(reference => reference.Name)
            .Order(StringComparer.Ordinal)
            .ToArray();

        Assert.Equal(["NTech.NVision.Core"], dependencies);
    }

    [Fact]
    public void EngineProjectsDoNotReferenceApplications()
    {
        RepositoryModel model = RepositoryModel.Load();

        foreach (ProjectNode project in model.Projects.Where(project => project.RelativePath.StartsWith("engine/", StringComparison.OrdinalIgnoreCase)))
        {
            foreach (string referencePath in project.ProjectReferences)
            {
                ProjectNode dependency = model.GetProjectByPath(referencePath);
                Assert.False(
                    dependency.RelativePath.StartsWith("apps/", StringComparison.OrdinalIgnoreCase),
                    $"Engine project {project.Name} must not reference application project {dependency.Name}.");
            }
        }
    }

    [Fact]
    public void EnginesReferenceOnlyPluginAbstractions()
    {
        RepositoryModel model = RepositoryModel.Load();

        foreach (ProjectNode project in model.Projects.Where(project => project.RelativePath.StartsWith("engine/", StringComparison.OrdinalIgnoreCase)))
        {
            foreach (string referencePath in project.ProjectReferences)
            {
                ProjectNode dependency = model.GetProjectByPath(referencePath);
                if (!dependency.RelativePath.StartsWith("plugins/", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                Assert.Equal("NTech.NVision.Plugin.Abstractions", dependency.Name);
            }
        }
    }

    [Fact]
    public void PackageVersionsAreManagedCentrally()
    {
        RepositoryModel model = RepositoryModel.Load();

        foreach (ProjectNode project in model.Projects)
        {
            XDocument document = XDocument.Load(project.FullPath);
            IEnumerable<XElement> versionedReferences = document
                .Descendants()
                .Where(element => element.Name.LocalName == "PackageReference")
                .Where(element => element.Attribute("Version") is not null || element.Elements().Any(child => child.Name.LocalName == "Version"));

            Assert.Empty(versionedReferences);
        }
    }
}

internal sealed class RepositoryModel
{
    private readonly Dictionary<string, ProjectNode> projectsByPath;

    private RepositoryModel(string rootPath, IReadOnlyList<ProjectNode> projects)
    {
        RootPath = rootPath;
        Projects = projects;
        ProjectsByName = projects.ToDictionary(project => project.Name, StringComparer.Ordinal);
        projectsByPath = projects.ToDictionary(project => project.FullPath, StringComparer.OrdinalIgnoreCase);
    }

    public string RootPath { get; }

    public IReadOnlyList<ProjectNode> Projects { get; }

    public IReadOnlyDictionary<string, ProjectNode> ProjectsByName { get; }

    public static RepositoryModel Load()
    {
        string rootPath = FindRepositoryRoot();
        string[] ignoredSegments = ["bin", "obj", ".git"];
        IReadOnlyList<ProjectNode> projects = Directory
            .EnumerateFiles(rootPath, "*.csproj", SearchOption.AllDirectories)
            .Where(path => !HasIgnoredSegment(path, ignoredSegments))
            .Select(path => ProjectNode.Load(rootPath, path))
            .OrderBy(project => project.RelativePath, StringComparer.Ordinal)
            .ToArray();

        return new RepositoryModel(rootPath, projects);
    }

    public ProjectNode GetProject(string name) => ProjectsByName[name];

    public ProjectNode GetProjectByPath(string path) => projectsByPath[Path.GetFullPath(path)];

    public string? FindCycle()
    {
        var states = new Dictionary<string, VisitState>(StringComparer.OrdinalIgnoreCase);
        var stack = new Stack<ProjectNode>();

        foreach (ProjectNode project in Projects)
        {
            string? cycle = Visit(project, states, stack);
            if (cycle is not null)
            {
                return cycle;
            }
        }

        return null;
    }

    private string? Visit(ProjectNode project, Dictionary<string, VisitState> states, Stack<ProjectNode> stack)
    {
        if (states.TryGetValue(project.FullPath, out VisitState state))
        {
            if (state == VisitState.Visited)
            {
                return null;
            }

            IEnumerable<string> cycleProjects = stack
                .Reverse()
                .SkipWhile(item => !string.Equals(item.FullPath, project.FullPath, StringComparison.OrdinalIgnoreCase))
                .Select(item => item.Name)
                .Append(project.Name);
            return string.Join(" -> ", cycleProjects);
        }

        states[project.FullPath] = VisitState.Visiting;
        stack.Push(project);

        foreach (string referencePath in project.ProjectReferences)
        {
            if (!projectsByPath.TryGetValue(Path.GetFullPath(referencePath), out ProjectNode? dependency))
            {
                throw new InvalidOperationException($"Project {project.Name} references a project outside the repository model: {referencePath}");
            }

            string? cycle = Visit(dependency, states, stack);
            if (cycle is not null)
            {
                return cycle;
            }
        }

        _ = stack.Pop();
        states[project.FullPath] = VisitState.Visited;
        return null;
    }

    private static string FindRepositoryRoot()
    {
        DirectoryInfo? directory = new(AppContext.BaseDirectory);
        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "NTech.NVision.sln")))
            {
                return directory.FullName;
            }

            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate the NVision repository root.");
    }

    private static bool HasIgnoredSegment(string path, IEnumerable<string> ignoredSegments)
    {
        string[] segments = Path.GetFullPath(path).Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        return segments.Any(segment => ignoredSegments.Contains(segment, StringComparer.OrdinalIgnoreCase));
    }

    private enum VisitState
    {
        Visiting,
        Visited,
    }
}

internal sealed record ProjectNode(
    string Name,
    string FullPath,
    string RelativePath,
    IReadOnlyList<string> ProjectReferences)
{
    public static ProjectNode Load(string rootPath, string projectPath)
    {
        string fullPath = Path.GetFullPath(projectPath);
        XDocument document = XDocument.Load(fullPath);
        string projectDirectory = Path.GetDirectoryName(fullPath)
            ?? throw new InvalidOperationException($"Project has no parent directory: {fullPath}");
        string[] references = document
            .Descendants()
            .Where(element => element.Name.LocalName == "ProjectReference")
            .Select(element => element.Attribute("Include")?.Value)
            .Where(value => !string.IsNullOrWhiteSpace(value))
            .Select(value => Path.GetFullPath(Path.Combine(projectDirectory, value!)))
            .ToArray();

        string relativePath = Path.GetRelativePath(rootPath, fullPath).Replace('\\', '/');
        return new ProjectNode(Path.GetFileNameWithoutExtension(fullPath), fullPath, relativePath, references);
    }
}
