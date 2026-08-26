#include "nvision/core/workflow_node.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nvision::core {

namespace {

constexpr std::size_t maximum_display_name_length = 128U;
constexpr std::size_t maximum_detail_length = 1'024U;

bool is_valid(const DiagnosticSeverity severity) noexcept {
    return severity == DiagnosticSeverity::warning || severity == DiagnosticSeverity::error;
}

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](const unsigned char item) {
        return std::isspace(item) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char item) {
        return std::isspace(item) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

void validate_text(
    const std::string_view value,
    const std::size_t maximum_length,
    const std::string_view field_name) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field_name) + " must not be blank");
    }
    if (value.size() > maximum_length) {
        throw std::invalid_argument(std::string(field_name) + " exceeds its maximum length");
    }
    if (std::any_of(value.begin(), value.end(), [](const unsigned char item) {
            return item < 0x20U && item != '\t';
        })) {
        throw std::invalid_argument(std::string(field_name) + " contains a control character");
    }
}

}  // namespace

NodeParameterAssignment::NodeParameterAssignment(ParameterKey key, ParameterValue value)
    : key_(std::move(key)), value_(std::move(value)) {}

const ParameterKey& NodeParameterAssignment::key() const noexcept {
    return key_;
}

const ParameterValue& NodeParameterAssignment::value() const noexcept {
    return value_;
}

NodeConfiguration::NodeConfiguration(std::vector<NodeParameterAssignment> assignments)
    : assignments_(std::move(assignments)) {
    for (std::size_t index = 0U; index < assignments_.size(); ++index) {
        const auto duplicate = std::find_if(
            assignments_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            assignments_.end(),
            [this, index](const NodeParameterAssignment& candidate) {
                return candidate.key() == assignments_[index].key();
            });
        if (duplicate != assignments_.end()) {
            throw std::invalid_argument("node parameter assignment keys must be unique");
        }
    }
}

const std::vector<NodeParameterAssignment>& NodeConfiguration::assignments() const noexcept {
    return assignments_;
}

const ParameterValue* NodeConfiguration::find(const ParameterKey& key) const noexcept {
    const auto match = std::find_if(
        assignments_.begin(),
        assignments_.end(),
        [&key](const NodeParameterAssignment& assignment) { return assignment.key() == key; });
    return match == assignments_.end() ? nullptr : &match->value();
}

bool NodeConfiguration::contains(const ParameterKey& key) const noexcept {
    return find(key) != nullptr;
}

std::size_t NodeConfiguration::size() const noexcept {
    return assignments_.size();
}

WorkflowNode::WorkflowNode(
    WorkflowNodeId id,
    ToolTypeId tool_type_id,
    SemanticVersion tool_version,
    std::string display_name,
    NodeConfiguration configuration)
    : id_(std::move(id)),
      tool_type_id_(std::move(tool_type_id)),
      tool_version_(std::move(tool_version)),
      display_name_(trim(std::move(display_name))),
      configuration_(std::move(configuration)) {
    validate_text(display_name_, maximum_display_name_length, "workflow node display name");
}

const WorkflowNodeId& WorkflowNode::id() const noexcept {
    return id_;
}

const ToolTypeId& WorkflowNode::tool_type_id() const noexcept {
    return tool_type_id_;
}

const SemanticVersion& WorkflowNode::tool_version() const noexcept {
    return tool_version_;
}

const std::string& WorkflowNode::display_name() const noexcept {
    return display_name_;
}

const NodeConfiguration& WorkflowNode::configuration() const noexcept {
    return configuration_;
}

WorkflowDiagnostic::WorkflowDiagnostic(
    ResultCode code,
    const DiagnosticSeverity severity,
    std::string detail,
    std::optional<WorkflowNodeId> node_id,
    std::optional<ParameterKey> parameter_key,
    std::optional<PortKey> port_key)
    : code_(std::move(code)),
      severity_(severity),
      detail_(trim(std::move(detail))),
      node_id_(std::move(node_id)),
      parameter_key_(std::move(parameter_key)),
      port_key_(std::move(port_key)) {
    if (!is_valid(severity_)) {
        throw std::invalid_argument("diagnostic severity is unsupported");
    }
    validate_text(detail_, maximum_detail_length, "diagnostic detail");
}

WorkflowDiagnostic WorkflowDiagnostic::node(
    ResultCode code,
    const DiagnosticSeverity severity,
    std::string detail,
    WorkflowNodeId node_id) {
    return WorkflowDiagnostic(
        std::move(code),
        severity,
        std::move(detail),
        std::move(node_id),
        std::nullopt,
        std::nullopt);
}

WorkflowDiagnostic WorkflowDiagnostic::parameter(
    ResultCode code,
    const DiagnosticSeverity severity,
    std::string detail,
    WorkflowNodeId node_id,
    ParameterKey parameter_key) {
    return WorkflowDiagnostic(
        std::move(code),
        severity,
        std::move(detail),
        std::move(node_id),
        std::move(parameter_key),
        std::nullopt);
}

WorkflowDiagnostic WorkflowDiagnostic::port(
    ResultCode code,
    const DiagnosticSeverity severity,
    std::string detail,
    WorkflowNodeId node_id,
    PortKey port_key) {
    return WorkflowDiagnostic(
        std::move(code),
        severity,
        std::move(detail),
        std::move(node_id),
        std::nullopt,
        std::move(port_key));
}

WorkflowDiagnostic WorkflowDiagnostic::interface_parameter(
    ResultCode code,
    const DiagnosticSeverity severity,
    std::string detail,
    ParameterKey parameter_key) {
    return WorkflowDiagnostic(
        std::move(code),
        severity,
        std::move(detail),
        std::nullopt,
        std::move(parameter_key),
        std::nullopt);
}

const ResultCode& WorkflowDiagnostic::code() const noexcept {
    return code_;
}

DiagnosticSeverity WorkflowDiagnostic::severity() const noexcept {
    return severity_;
}

const std::string& WorkflowDiagnostic::detail() const noexcept {
    return detail_;
}

const std::optional<WorkflowNodeId>& WorkflowDiagnostic::node_id() const noexcept {
    return node_id_;
}

const std::optional<ParameterKey>& WorkflowDiagnostic::parameter_key() const noexcept {
    return parameter_key_;
}

const std::optional<PortKey>& WorkflowDiagnostic::port_key() const noexcept {
    return port_key_;
}

WorkflowNodeBinding::WorkflowNodeBinding(
    WorkflowNode node,
    const ToolResolutionStatus status,
    std::shared_ptr<const ToolTypeDefinition> definition,
    std::vector<WorkflowDiagnostic> diagnostics)
    : node_(std::move(node)),
      status_(status),
      definition_(std::move(definition)),
      diagnostics_(std::move(diagnostics)) {
    if ((status_ == ToolResolutionStatus::resolved) != static_cast<bool>(definition_)) {
        throw std::invalid_argument("resolved node status and tool definition must agree");
    }
    if (definition_ && (definition_->id() != node_.tool_type_id() ||
                        definition_->version() != node_.tool_version())) {
        throw std::invalid_argument("bound tool definition must match the persisted node identity");
    }
}

const WorkflowNode& WorkflowNodeBinding::node() const noexcept {
    return node_;
}

ToolResolutionStatus WorkflowNodeBinding::status() const noexcept {
    return status_;
}

bool WorkflowNodeBinding::is_resolved() const noexcept {
    return status_ == ToolResolutionStatus::resolved;
}

const std::shared_ptr<const ToolTypeDefinition>& WorkflowNodeBinding::definition() const noexcept {
    return definition_;
}

const std::vector<WorkflowDiagnostic>& WorkflowNodeBinding::diagnostics() const noexcept {
    return diagnostics_;
}

bool WorkflowNodeBinding::has_errors() const noexcept {
    return std::any_of(
        diagnostics_.begin(), diagnostics_.end(), [](const WorkflowDiagnostic& diagnostic) {
            return diagnostic.severity() == DiagnosticSeverity::error;
        });
}

WorkflowNodeBinding bind_workflow_node(WorkflowNode node, const ToolRegistry& registry) {
    const ToolResolution resolution =
        registry.resolve_exact(node.tool_type_id(), node.tool_version());
    std::vector<WorkflowDiagnostic> diagnostics;

    if (!resolution.is_resolved()) {
        // An unresolved node is reported, never repaired. Its configuration stays
        // untouched so authored intent survives a missing plugin or a version gap.
        const bool unknown = resolution.status() == ToolResolutionStatus::unknown_tool;
        diagnostics.push_back(WorkflowDiagnostic::node(
            ResultCode(unknown ? "UNKNOWN_TOOL_TYPE" : "UNSUPPORTED_TOOL_VERSION"),
            DiagnosticSeverity::error,
            unknown ? "tool type " + node.tool_type_id().value() + " is not registered"
                    : "tool type " + node.tool_type_id().value() + " does not provide version " +
                          node.tool_version().to_string(),
            node.id()));
        return WorkflowNodeBinding(
            std::move(node), resolution.status(), nullptr, std::move(diagnostics));
    }

    const auto definition = resolution.definition();
    for (const auto& assignment : node.configuration().assignments()) {
        const auto* parameter = definition->find_parameter(assignment.key());
        if (parameter == nullptr) {
            // Preserved for compatibility with a version that declared this parameter.
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                ResultCode("UNKNOWN_PARAMETER"),
                DiagnosticSeverity::warning,
                "parameter " + assignment.key().value() +
                    " is preserved but not declared by the bound tool version",
                node.id(),
                assignment.key()));
            continue;
        }
        if (!parameter->type().accepts(assignment.value())) {
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                ResultCode("PARAMETER_TYPE_MISMATCH"),
                DiagnosticSeverity::error,
                "parameter " + assignment.key().value() + " does not satisfy its declared schema",
                node.id(),
                assignment.key()));
        }
    }

    for (const auto& parameter : definition->parameters()) {
        if (!parameter.required() || parameter.default_value().has_value()) {
            continue;
        }
        if (!node.configuration().contains(parameter.key())) {
            diagnostics.push_back(WorkflowDiagnostic::parameter(
                ResultCode("MISSING_REQUIRED_PARAMETER"),
                DiagnosticSeverity::error,
                "required parameter " + parameter.key().value() +
                    " has no assignment and no default",
                node.id(),
                parameter.key()));
        }
    }

    return WorkflowNodeBinding(
        std::move(node), ToolResolutionStatus::resolved, definition, std::move(diagnostics));
}

}  // namespace nvision::core
