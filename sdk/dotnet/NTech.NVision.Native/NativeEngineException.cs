namespace NTech.NVision.Native;

public sealed class NativeEngineException : Exception
{
    internal NativeEngineException(NativeStatusCode status, string message)
        : base($"Native engine operation failed with status '{status}': {message}") =>
        Status = status;

    public NativeStatusCode Status { get; }

    public int StatusCode => (int)Status;
}
