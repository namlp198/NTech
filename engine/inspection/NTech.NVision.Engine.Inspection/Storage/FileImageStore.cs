using System.Security.Cryptography;
using NTech.NVision.Contracts.Imaging;
using NTech.NVision.Contracts.Inspection;

namespace NTech.NVision.Engine.Inspection.Storage;

public sealed class FileImageStore
{
    private readonly string root;

    public FileImageStore(string root)
    {
        if (string.IsNullOrWhiteSpace(root))
        {
            throw new ArgumentException("Image store root is required.", nameof(root));
        }

        this.root = Path.GetFullPath(root);
        Directory.CreateDirectory(this.root);
    }

    public async ValueTask<ImageEvidence> StoreAsync(
        string inspectionId,
        Frame frame,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(inspectionId);
        ArgumentNullException.ThrowIfNull(frame);
        string directory = Path.Combine(root, inspectionId);
        Directory.CreateDirectory(directory);
        string sourceName = frame.Metadata.TryGetValue("sourceFile", out string? value)
            ? Path.GetFileName(value)
            : "source.raw";
        string target = Path.Combine(directory, sourceName);
        byte[] content;
        if (frame.Metadata.TryGetValue("sourcePath", out string? sourcePath) && File.Exists(sourcePath))
        {
            content = await File.ReadAllBytesAsync(sourcePath, cancellationToken).ConfigureAwait(false);
        }
        else
        {
            content = frame.Buffer.Memory.ToArray();
        }

        await File.WriteAllBytesAsync(target, content, cancellationToken).ConfigureAwait(false);
        string checksum = Convert.ToHexString(SHA256.HashData(content)).ToLowerInvariant();
        return new ImageEvidence("source", target, frame.Id.ToString(), checksum);
    }
}
