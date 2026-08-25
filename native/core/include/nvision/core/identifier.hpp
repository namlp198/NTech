#pragma once

#include <compare>
#include <string>
#include <string_view>
#include <utility>

namespace nvision::core {

namespace detail {

void validate_stable_identifier(std::string_view value);

}  // namespace detail

template <typename Tag>
class StableIdentifier final {
public:
    explicit StableIdentifier(std::string value) : value_(std::move(value)) {
        detail::validate_stable_identifier(value_);
    }

    [[nodiscard]] const std::string& value() const noexcept {
        return value_;
    }

    [[nodiscard]] auto operator<=>(const StableIdentifier&) const noexcept = default;

private:
    std::string value_;
};

struct ProjectIdTag final {};
struct WorkflowIdTag final {};
struct JobIdTag final {};
struct RecipeIdTag final {};
struct ExecutionSessionIdTag final {};
struct ResourceIdTag final {};
struct CoordinateSpaceIdTag final {};

using ProjectId = StableIdentifier<ProjectIdTag>;
using WorkflowId = StableIdentifier<WorkflowIdTag>;
using JobId = StableIdentifier<JobIdTag>;
using RecipeId = StableIdentifier<RecipeIdTag>;
using ExecutionSessionId = StableIdentifier<ExecutionSessionIdTag>;
using ResourceId = StableIdentifier<ResourceIdTag>;
using CoordinateSpaceId = StableIdentifier<CoordinateSpaceIdTag>;

}  // namespace nvision::core
