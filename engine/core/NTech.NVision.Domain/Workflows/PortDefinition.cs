using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record PortDefinition
{
    public PortDefinition(
        PortKey key,
        string name,
        PortDirection direction,
        PortDataType dataType,
        bool required = false,
        bool multiConnectionAllowed = false)
    {
        Key = key ?? throw new ArgumentNullException(nameof(key));
        Name = DomainText.Require(name, nameof(name));
        Direction = direction;
        DataType = dataType ?? throw new ArgumentNullException(nameof(dataType));
        Required = required;
        MultiConnectionAllowed = multiConnectionAllowed;
    }

    public PortKey Key { get; }

    public string Name { get; }

    public PortDirection Direction { get; }

    public PortDataType DataType { get; }

    public bool Required { get; }

    public bool MultiConnectionAllowed { get; }
}
