namespace NTech.NVision.Core.Errors;

/// <summary>Represents a stable, structured failure that can cross module boundaries.</summary>
public sealed record DomainError
{
    public DomainError(string code, string message)
    {
        if (string.IsNullOrWhiteSpace(code))
        {
            throw new ArgumentException("Error code is required.", nameof(code));
        }

        if (string.IsNullOrWhiteSpace(message))
        {
            throw new ArgumentException("Error message is required.", nameof(message));
        }

        Code = code.Trim();
        Message = message.Trim();
    }

    public string Code { get; }

    public string Message { get; }
}
