using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Plugins;

public sealed record PluginId
{
    public PluginId(string value) => Value = StrongIdentifierGuard.RequireStableCode(value, nameof(value));

    public string Value { get; }

    public static PluginId Parse(string value) => new(value);

    public override string ToString() => Value;
}
