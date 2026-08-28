#include "nvision/workflow/tool_execution.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>

namespace nvision::workflow {

namespace {

using core::ErrorCategory;
using core::ExecutionError;
using core::ExecutionStatus;
using core::ResultCode;

bool is_terminal_failure(const ExecutionStatus status) noexcept {
    return status == ExecutionStatus::error || status == ExecutionStatus::timeout ||
           status == ExecutionStatus::cancelled;
}

ExecutionError contract_violation(std::string detail) {
    return ExecutionError(
        ResultCode("OUTPUT_CONTRACT_VIOLATION"), ErrorCategory::workflow, std::move(detail), false);
}

}  // namespace

CancellationToken::CancellationToken(std::shared_ptr<const std::atomic_bool> state) noexcept
    : state_(std::move(state)) {}

CancellationToken CancellationToken::none() {
    return CancellationToken(nullptr);
}

bool CancellationToken::is_cancellation_requested() const noexcept {
    return state_ != nullptr && state_->load(std::memory_order_acquire);
}

CancellationSource::CancellationSource() : state_(std::make_shared<std::atomic_bool>(false)) {}

void CancellationSource::request_cancellation() noexcept {
    state_->store(true, std::memory_order_release);
}

bool CancellationSource::is_cancellation_requested() const noexcept {
    return state_->load(std::memory_order_acquire);
}

CancellationToken CancellationSource::token() const noexcept {
    return CancellationToken(state_);
}

ExecutionBudget::ExecutionBudget(const std::chrono::nanoseconds budget) noexcept
    : budget_(budget) {}

ExecutionBudget ExecutionBudget::unbounded() noexcept {
    return ExecutionBudget(std::chrono::nanoseconds::zero());
}

ExecutionBudget ExecutionBudget::of(const std::chrono::nanoseconds budget) {
    if (budget <= std::chrono::nanoseconds::zero()) {
        throw std::invalid_argument("a bounded execution budget must be positive");
    }
    return ExecutionBudget(budget);
}

bool ExecutionBudget::is_bounded() const noexcept {
    return budget_ > std::chrono::nanoseconds::zero();
}

std::chrono::nanoseconds ExecutionBudget::budget() const noexcept {
    return budget_;
}

bool ExecutionBudget::is_exceeded_by(const std::chrono::nanoseconds elapsed) const noexcept {
    return is_bounded() && elapsed > budget_;
}

ParameterBinding::ParameterBinding(std::vector<Entry> values) noexcept : values_(std::move(values)) {}

ParameterBinding ParameterBinding::resolve(
    const ToolTypeDefinition& definition,
    const NodeConfiguration& configuration) {
    std::vector<Entry> values;
    values.reserve(definition.parameters().size());
    for (const auto& parameter : definition.parameters()) {
        const ParameterValue* assigned = configuration.find(parameter.key());
        if (assigned != nullptr) {
            if (!parameter.type().accepts(*assigned)) {
                throw std::invalid_argument(
                    "parameter " + parameter.key().value() +
                    " does not satisfy its declared schema");
            }
            values.emplace_back(parameter.key(), *assigned);
            continue;
        }
        if (parameter.default_value().has_value()) {
            values.emplace_back(parameter.key(), *parameter.default_value());
            continue;
        }
        if (parameter.required()) {
            throw std::invalid_argument(
                "required parameter " + parameter.key().value() +
                " has no assignment and no default");
        }
    }
    return ParameterBinding(std::move(values));
}

const ParameterValue& ParameterBinding::value(const core::ParameterKey& key) const {
    const ParameterValue* found = find(key);
    if (found == nullptr) {
        throw std::invalid_argument("parameter " + key.value() + " is not bound");
    }
    return *found;
}

const ParameterValue* ParameterBinding::find(const core::ParameterKey& key) const noexcept {
    const auto match =
        std::find_if(values_.begin(), values_.end(), [&key](const Entry& candidate) {
            return candidate.first == key;
        });
    return match == values_.end() ? nullptr : &match->second;
}

std::size_t ParameterBinding::size() const noexcept {
    return values_.size();
}

const std::vector<ParameterBinding::Entry>& ParameterBinding::entries() const noexcept {
    return values_;
}

ToolExecutionContext::ToolExecutionContext(
    std::shared_ptr<const ToolTypeDefinition> definition,
    core::ExecutionSessionId session_id,
    core::WorkflowNodeId node_id,
    PortValueMap inputs,
    ParameterBinding parameters,
    CancellationToken cancellation,
    ExecutionBudget budget,
    EvidenceSink* evidence_sink)
    : definition_(std::move(definition)),
      session_id_(std::move(session_id)),
      node_id_(std::move(node_id)),
      inputs_(std::move(inputs)),
      parameters_(std::move(parameters)),
      cancellation_(std::move(cancellation)),
      budget_(budget),
      evidence_sink_(evidence_sink) {
    if (!definition_) {
        throw std::invalid_argument("a tool execution context requires a bound tool definition");
    }
    for (const auto& entry : inputs_.entries()) {
        const auto* port = definition_->find_port(entry.first, PortDirection::input);
        if (port == nullptr) {
            throw std::invalid_argument(
                "port " + entry.first.value() + " is not a declared input of tool " +
                definition_->id().value());
        }
        if (port->data_type() != entry.second.type()) {
            throw std::invalid_argument(
                "input port " + entry.first.value() + " of tool " + definition_->id().value() +
                " received a value of a different data kind");
        }
    }
    for (const auto& port : definition_->ports()) {
        if (port.direction() != PortDirection::input || !port.required()) {
            continue;
        }
        if (!inputs_.contains(port.key())) {
            throw std::invalid_argument(
                "required input port " + port.key().value() + " of tool " +
                definition_->id().value() + " has no value");
        }
    }
}

const ToolTypeDefinition& ToolExecutionContext::definition() const noexcept {
    return *definition_;
}

const core::ExecutionSessionId& ToolExecutionContext::session_id() const noexcept {
    return session_id_;
}

const core::WorkflowNodeId& ToolExecutionContext::node_id() const noexcept {
    return node_id_;
}

const PortValueMap& ToolExecutionContext::inputs() const noexcept {
    return inputs_;
}

const ParameterBinding& ToolExecutionContext::parameters() const noexcept {
    return parameters_;
}

const ExecutionBudget& ToolExecutionContext::budget() const noexcept {
    return budget_;
}

EvidenceSink* ToolExecutionContext::evidence_sink() const noexcept {
    return evidence_sink_;
}

bool ToolExecutionContext::is_cancellation_requested() const noexcept {
    return cancellation_.is_cancellation_requested();
}

const PortValue& ToolExecutionContext::input(const core::PortKey& key) const {
    const PortValue* value = inputs_.find(key);
    if (value == nullptr) {
        throw std::invalid_argument("input port " + key.value() + " has no value");
    }
    return *value;
}

const ParameterValue& ToolExecutionContext::parameter(const core::ParameterKey& key) const {
    return parameters_.value(key);
}

core::ImageId ToolExecutionContext::output_image_id(const core::PortKey& output_port) const {
    std::string identity = node_id_.value() + "." + output_port.value();
    if (identity.size() > acquisition::maximum_image_identity_length) {
        throw std::invalid_argument(
            "node " + node_id_.value() +
            " has a name too long to derive an output image identity");
    }
    return core::ImageId(std::move(identity));
}

core::EvidenceId ToolExecutionContext::output_evidence_id(
    const core::PortKey& output_port) const {
    return core::EvidenceId(node_id_.value() + "." + output_port.value());
}

ToolExecutionOutcome::ToolExecutionOutcome(
    const ExecutionStatus status,
    PortValueMap outputs,
    std::vector<ExecutionError> errors)
    : status_(status), outputs_(std::move(outputs)), errors_(std::move(errors)) {
    if (status_ == ExecutionStatus::success) {
        if (!errors_.empty()) {
            throw std::invalid_argument("a successful tool outcome must not carry errors");
        }
        return;
    }
    if (!is_terminal_failure(status_)) {
        throw std::invalid_argument("tool execution status is unsupported");
    }
    if (errors_.empty()) {
        throw std::invalid_argument("a failed tool outcome must carry at least one error");
    }
    if (!outputs_.empty()) {
        throw std::invalid_argument("a failed tool outcome must not carry outputs");
    }
}

ToolExecutionOutcome ToolExecutionOutcome::succeeded(PortValueMap outputs) {
    return ToolExecutionOutcome(ExecutionStatus::success, std::move(outputs), {});
}

ToolExecutionOutcome ToolExecutionOutcome::failed(ExecutionError error) {
    std::vector<ExecutionError> errors;
    errors.push_back(std::move(error));
    return ToolExecutionOutcome(ExecutionStatus::error, {}, std::move(errors));
}

ToolExecutionOutcome ToolExecutionOutcome::timed_out(ExecutionError error) {
    std::vector<ExecutionError> errors;
    errors.push_back(std::move(error));
    return ToolExecutionOutcome(ExecutionStatus::timeout, {}, std::move(errors));
}

ToolExecutionOutcome ToolExecutionOutcome::cancelled(ExecutionError error) {
    std::vector<ExecutionError> errors;
    errors.push_back(std::move(error));
    return ToolExecutionOutcome(ExecutionStatus::cancelled, {}, std::move(errors));
}

ExecutionStatus ToolExecutionOutcome::status() const noexcept {
    return status_;
}

bool ToolExecutionOutcome::is_success() const noexcept {
    return status_ == ExecutionStatus::success;
}

const PortValueMap& ToolExecutionOutcome::outputs() const noexcept {
    return outputs_;
}

const std::vector<ExecutionError>& ToolExecutionOutcome::errors() const noexcept {
    return errors_;
}

namespace {

// Returns the first reason the produced outputs do not satisfy the declared output
// ports, or an empty string when they do.
std::string describe_output_violation(
    const ToolTypeDefinition& definition,
    const PortValueMap& outputs) {
    for (const auto& entry : outputs.entries()) {
        const auto* port = definition.find_port(entry.first, PortDirection::output);
        if (port == nullptr) {
            return "port " + entry.first.value() + " is not a declared output of tool " +
                   definition.id().value();
        }
        if (port->data_type() != entry.second.type()) {
            return "output port " + entry.first.value() + " of tool " + definition.id().value() +
                   " produced a value of a different data kind";
        }
    }
    for (const auto& port : definition.ports()) {
        if (port.direction() != PortDirection::output || !port.required()) {
            continue;
        }
        if (!outputs.contains(port.key())) {
            return "required output port " + port.key().value() + " of tool " +
                   definition.id().value() + " was not produced";
        }
    }
    return {};
}

}  // namespace

ToolExecutionOutcome execute_tool(
    const ToolImplementation& implementation,
    const ToolExecutionContext& context) {
    const ToolTypeDefinition& definition = context.definition();
    if (implementation.tool_type_id() != definition.id() ||
        implementation.tool_version() != definition.version()) {
        throw std::invalid_argument(
            "tool implementation identity must match the bound tool definition");
    }

    if (context.is_cancellation_requested()) {
        return ToolExecutionOutcome::cancelled(ExecutionError(
            ResultCode("EXECUTION_CANCELLED"),
            ErrorCategory::workflow,
            "tool " + definition.id().value() + " was cancelled before it started",
            true));
    }

    const auto started_at = std::chrono::steady_clock::now();
    ToolExecutionOutcome outcome = ToolExecutionOutcome::failed(ExecutionError(
        ResultCode("TOOL_EXECUTION_FAULT"),
        ErrorCategory::workflow,
        "tool " + definition.id().value() + " did not produce an outcome",
        false));
    try {
        outcome = implementation.execute(context);
    } catch (const std::exception& error) {
        // A tool must not unwind into the engine. Its failure becomes a technical error
        // so the run stays Indeterminate instead of ending as OK or NG.
        return ToolExecutionOutcome::failed(ExecutionError(
            ResultCode("TOOL_EXECUTION_FAULT"),
            ErrorCategory::workflow,
            "tool " + definition.id().value() + " raised: " + error.what(),
            false));
    }
    const auto elapsed = std::chrono::steady_clock::now() - started_at;

    if (context.budget().is_exceeded_by(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed))) {
        return ToolExecutionOutcome::timed_out(ExecutionError(
            ResultCode("EXECUTION_TIMEOUT"),
            ErrorCategory::workflow,
            "tool " + definition.id().value() + " exceeded its execution budget",
            true));
    }

    if (!outcome.is_success()) {
        return outcome;
    }

    const std::string violation = describe_output_violation(definition, outcome.outputs());
    if (!violation.empty()) {
        return ToolExecutionOutcome::failed(contract_violation(violation));
    }
    return outcome;
}

}  // namespace nvision::workflow
