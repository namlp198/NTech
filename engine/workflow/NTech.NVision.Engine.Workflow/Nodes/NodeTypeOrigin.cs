using NTech.NVision.Domain.Plugins;

namespace NTech.NVision.Engine.Workflow.Nodes;

public sealed record NodeTypeOrigin
{
    private NodeTypeOrigin(bool isBuiltIn, PluginId? pluginId)
    {
        IsBuiltIn = isBuiltIn;
        PluginId = pluginId;
    }

    public bool IsBuiltIn { get; }

    public PluginId? PluginId { get; }

    public static NodeTypeOrigin BuiltIn { get; } = new(true, null);

    public static NodeTypeOrigin FromPlugin(PluginId pluginId) =>
        new(false, pluginId ?? throw new ArgumentNullException(nameof(pluginId)));
}
