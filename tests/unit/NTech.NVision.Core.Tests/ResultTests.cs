using NTech.NVision.Core.Errors;
using NTech.NVision.Core.Results;

namespace NTech.NVision.Core.Tests;

public sealed class ResultTests
{
    [Fact]
    public void SuccessExposesValue()
    {
        Result<string> result = Result.Success("ready");

        Assert.True(result.IsSuccess);
        Assert.False(result.IsFailure);
        Assert.Equal("ready", result.Value);
        Assert.Null(result.Error);
    }

    [Fact]
    public void FailureExposesStructuredError()
    {
        var error = new DomainError("workflow.invalid", "Workflow validation failed.");

        Result<string> result = Result.Failure<string>(error);

        Assert.True(result.IsFailure);
        Assert.Same(error, result.Error);
    }

    [Fact]
    public void FailureValueAccessThrows()
    {
        Result<string> result = Result.Failure<string>(new DomainError("workflow.invalid", "Workflow validation failed."));

        Assert.Throws<InvalidOperationException>(() => result.Value);
    }

    [Theory]
    [InlineData("")]
    [InlineData("   ")]
    public void ErrorRejectsBlankCode(string code)
    {
        Assert.Throws<ArgumentException>(() => new DomainError(code, "Message"));
    }
}
