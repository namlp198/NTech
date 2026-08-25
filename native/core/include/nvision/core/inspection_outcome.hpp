#pragma once

#include <cstdint>
#include <string>

namespace nvision::core {

enum class ExecutionStatus : std::uint32_t {
    success = 0,
    error = 1,
    timeout = 2,
    cancelled = 3,
};

enum class InspectionVerdict : std::uint32_t {
    ok = 0,
    ng = 1,
    indeterminate = 2,
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
