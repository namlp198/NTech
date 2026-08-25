#pragma once

#include <cstdint>
#include <string>

namespace nvision::core {

enum class ExecutionStatus : std::uint32_t {
    success,
    error,
    timeout,
    cancelled,
};

enum class InspectionVerdict : std::uint32_t {
    ok,
    ng,
    indeterminate,
};

class InspectionOutcome final {
public:
    InspectionOutcome(
        ExecutionStatus execution_status,
        InspectionVerdict verdict,
        std::string error_code = {});

    [[nodiscard]] ExecutionStatus execution_status() const noexcept;
    [[nodiscard]] InspectionVerdict verdict() const noexcept;
    [[nodiscard]] const std::string& error_code() const noexcept;

private:
    ExecutionStatus execution_status_;
    InspectionVerdict verdict_;
    std::string error_code_;
};

}  // namespace nvision::core
