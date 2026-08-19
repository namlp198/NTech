using NTech.NVision.Domain.Common;

namespace NTech.NVision.Domain.Workflows;

public sealed record PortDataType
{
    private const string Prefix = "nvision.data.";

    public PortDataType(string value) => Value = StrongIdentifierGuard.RequireStableCode(value, nameof(value));

    public string Value { get; }

    public static PortDataType Image { get; } = BuiltIn("image");

    public static PortDataType Mask { get; } = BuiltIn("mask");

    public static PortDataType Region { get; } = BuiltIn("region");

    public static PortDataType Point2D { get; } = BuiltIn("point2d");

    public static PortDataType Point3D { get; } = BuiltIn("point3d");

    public static PortDataType Line2D { get; } = BuiltIn("line2d");

    public static PortDataType Circle { get; } = BuiltIn("circle");

    public static PortDataType Contour { get; } = BuiltIn("contour");

    public static PortDataType BoundingBox { get; } = BuiltIn("bounding-box");

    public static PortDataType Detection { get; } = BuiltIn("detection");

    public static PortDataType DetectionList { get; } = BuiltIn("detection-list");

    public static PortDataType Classification { get; } = BuiltIn("classification");

    public static PortDataType TextResult { get; } = BuiltIn("text-result");

    public static PortDataType Measurement { get; } = BuiltIn("measurement");

    public static PortDataType Boolean { get; } = BuiltIn("boolean");

    public static PortDataType IntegerNumber { get; } = BuiltIn("integer");

    public static PortDataType FloatingPoint { get; } = BuiltIn("float");

    public static PortDataType Text { get; } = BuiltIn("string");

    public static PortDataType Pose { get; } = BuiltIn("pose");

    public static PortDataType Transform { get; } = BuiltIn("transform");

    public static PortDataType Decision { get; } = BuiltIn("decision");

    public static PortDataType AnyMetadata { get; } = BuiltIn("any-metadata");

    public static PortDataType Parse(string value) => new(value);

    public override string ToString() => Value;

    private static PortDataType BuiltIn(string name) => new(Prefix + name);
}
