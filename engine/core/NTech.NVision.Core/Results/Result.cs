using NTech.NVision.Core.Errors;

namespace NTech.NVision.Core.Results;

/// <summary>Represents either a successful value or an explicit structured failure.</summary>
public sealed class Result<T>
{
    private readonly T? value;

    internal Result(T value)
    {
        ArgumentNullException.ThrowIfNull(value);
        this.value = value;
        IsSuccess = true;
    }

    internal Result(DomainError error)
    {
        Error = error ?? throw new ArgumentNullException(nameof(error));
        IsSuccess = false;
    }

    public bool IsSuccess { get; }

    public bool IsFailure => !IsSuccess;

    public T Value => IsSuccess
        ? value!
        : throw new InvalidOperationException("A failed result does not contain a value.");

    public DomainError? Error { get; }
}

public static class Result
{
    public static Result<T> Success<T>(T value) => new(value);

    public static Result<T> Failure<T>(DomainError error) => new(error);
}
