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
struct WorkflowNodeIdTag final {};
struct JobIdTag final {};
struct RecipeIdTag final {};
struct ExecutionSessionIdTag final {};
struct InspectionIdTag final {};
struct ResourceIdTag final {};
struct ImageIdTag final {};
struct CoordinateSpaceIdTag final {};
struct EvidenceIdTag final {};
struct ToolTypeIdTag final {};
struct PluginIdTag final {};
struct PortKeyTag final {};
struct ParameterKeyTag final {};
struct EnumOptionKeyTag final {};

using ProjectId = StableIdentifier<ProjectIdTag>;
using WorkflowId = StableIdentifier<WorkflowIdTag>;
using WorkflowNodeId = StableIdentifier<WorkflowNodeIdTag>;
using JobId = StableIdentifier<JobIdTag>;
using RecipeId = StableIdentifier<RecipeIdTag>;
using ExecutionSessionId = StableIdentifier<ExecutionSessionIdTag>;
using InspectionId = StableIdentifier<InspectionIdTag>;
using ResourceId = StableIdentifier<ResourceIdTag>;
using ImageId = StableIdentifier<ImageIdTag>;
using CoordinateSpaceId = StableIdentifier<CoordinateSpaceIdTag>;
using EvidenceId = StableIdentifier<EvidenceIdTag>;
using ToolTypeId = StableIdentifier<ToolTypeIdTag>;
using PluginId = StableIdentifier<PluginIdTag>;
using PortKey = StableIdentifier<PortKeyTag>;
using ParameterKey = StableIdentifier<ParameterKeyTag>;
using EnumOptionKey = StableIdentifier<EnumOptionKeyTag>;

}  // namespace nvision::core
