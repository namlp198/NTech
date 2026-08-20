namespace NTech.NVision.Native;

public sealed class NativeEngineException : Exception
{
    internal NativeEngineException(NativeStatus status, string message)
        : base($"Native engine operation failed with status '{status}': {message}") =>
        StatusCode = (int)status;

    public int StatusCode { get; }
}
