#pragma once

#include "nvision/core/execution_result.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/core/inspection_outcome.hpp"
#include "nvision/core/semantic_version.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/port_value.hpp"
#include "nvision/workflow/tool_definition.hpp"
#include "nvision/workflow/workflow_node.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace nvision::workflow {

// Observer side of cooperative cancellation. Copies share one request flag, so a
// supervisor can cancel a run while the tools executing it only ever read the flag.
class CancellationToken final {
public:
    [[nodiscard]] static CancellationToken none();

    [[nodiscard]] bool is_cancellation_requested() const noexcept;

private:
    friend class CancellationSource;

    explicit CancellationToken(std::shared_ptr<const std::atomic_bool> state) noexcept;

    std::shared_ptr<const std::atomic_bool> state_;
};

// Owner side of cooperative cancellation.
class CancellationSource final {
public:
    CancellationSource();

    void request_cancellation() noexcept;
    [[nodiscard]] bool is_cancellation_requested() const noexcept;
    [[nodiscard]] CancellationToken token() const noexcept;

private:
    std::shared_ptr<std::atomic_bool> state_;
};

// The monotonic time a single tool invocation is allowed to take. An unbounded budget
// means the caller accepts whatever the tool needs; it is not a hidden default timeout.
class ExecutionBudget final {
public:
    [[nodiscard]] static ExecutionBudget unbounded() noexcept;
    [[nodiscard]] static ExecutionBudget of(std::chrono::nanoseconds budget);

    [[nodiscard]] bool is_bounded() const noexcept;
    [[nodiscard]] std::chrono::nanoseconds budget() const noexcept;
    [[nodiscard]] bool is_exceeded_by(std::chrono::nanoseconds elapsed) const noexcept;

private:
    explicit ExecutionBudget(std::chrono::nanoseconds budget) noexcept;

    std::chrono::nanoseconds budget_;
};

// The effective parameter values of one node: every parameter the bound tool version
// declares, taken from the node configuration or from the declared default.
//
// Configuration keys the bound version does not declare are preserved on the node but
// are not part of the binding, because a tool must never receive a value it did not
// declare. Resolution is strict: it expects a configuration that bind_workflow_node
// already reported as free of errors.
class ParameterBinding final {
public:
    using Entry = std::pair<core::ParameterKey, ParameterValue>;

    [[nodiscard]] static ParameterBinding resolve(
        const ToolTypeDefinition& definition,
        const NodeConfiguration& configuration);

    [[nodiscard]] const ParameterValue& value(const core::ParameterKey& key) const;
    [[nodiscard]] const ParameterValue* find(const core::ParameterKey& key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const std::vector<Entry>& entries() const noexcept;

private:
    explicit ParameterBinding(std::vector<Entry> values) noexcept;

    std::vector<Entry> values_;
};

// Everything one tool invocation may read. Construction validates the inputs against
// the bound tool definition, so an implementation never has to re-check that a port
// exists, is declared as an input, carries the declared data kind, or is present when
// the contract marks it required.
class ToolExecutionContext final {
public:
    ToolExecutionContext(
        std::shared_ptr<const ToolTypeDefinition> definition,
        core::ExecutionSessionId session_id,
        core::WorkflowNodeId node_id,
        PortValueMap inputs,
        ParameterBinding parameters,
        CancellationToken cancellation = CancellationToken::none(),
        ExecutionBudget budget = ExecutionBudget::unbounded());

    [[nodiscard]] const ToolTypeDefinition& definition() const noexcept;
    [[nodiscard]] const core::ExecutionSessionId& session_id() const noexcept;
    [[nodiscard]] const core::WorkflowNodeId& node_id() const noexcept;
    [[nodiscard]] const PortValueMap& inputs() const noexcept;
    [[nodiscard]] const ParameterBinding& parameters() const noexcept;
    [[nodiscard]] const ExecutionBudget& budget() const noexcept;
    [[nodiscard]] bool is_cancellation_requested() const noexcept;

    [[nodiscard]] const PortValue& input(const core::PortKey& key) const;
    [[nodiscard]] const ParameterValue& parameter(const core::ParameterKey& key) const;

    // The identity a tool must give an image it produces on one of its output ports.
    // Deriving it from the node and the port rather than from a counter keeps a repeated
    // run reproducing the same coordinate space for the same output.
    [[nodiscard]] core::ImageId output_image_id(const core::PortKey& output_port) const;

private:
    std::shared_ptr<const ToolTypeDefinition> definition_;
    core::ExecutionSessionId session_id_;
    core::WorkflowNodeId node_id_;
    PortValueMap inputs_;
    ParameterBinding parameters_;
    CancellationToken cancellation_;
    ExecutionBudget budget_;
};

// The technical result of one tool invocation. A non-success status always carries at
// least one error and never carries outputs, so a failed tool cannot hand a downstream
// node a value that looks valid.
class ToolExecutionOutcome final {
public:
    [[nodiscard]] static ToolExecutionOutcome succeeded(PortValueMap outputs);
    [[nodiscard]] static ToolExecutionOutcome failed(core::ExecutionError error);
    [[nodiscard]] static ToolExecutionOutcome timed_out(core::ExecutionError error);
    [[nodiscard]] static ToolExecutionOutcome cancelled(core::ExecutionError error);

    [[nodiscard]] core::ExecutionStatus status() const noexcept;
    [[nodiscard]] bool is_success() const noexcept;
    [[nodiscard]] const PortValueMap& outputs() const noexcept;
    [[nodiscard]] const std::vector<core::ExecutionError>& errors() const noexcept;

private:
    ToolExecutionOutcome(
        core::ExecutionStatus status,
        PortValueMap outputs,
        std::vector<core::ExecutionError> errors);

    core::ExecutionStatus status_;
    PortValueMap outputs_;
    std::vector<core::ExecutionError> errors_;
};

// The native side of a tool. Implementations are shared and executed concurrently, so
// execute() is const and any per-run state belongs in the context or in local storage.
class ToolImplementation {
public:
    ToolImplementation() = default;
    ToolImplementation(const ToolImplementation&) = delete;
    ToolImplementation& operator=(const ToolImplementation&) = delete;
    ToolImplementation(ToolImplementation&&) = delete;
    ToolImplementation& operator=(ToolImplementation&&) = delete;
    virtual ~ToolImplementation() = default;

    [[nodiscard]] virtual const core::ToolTypeId& tool_type_id() const noexcept = 0;
    [[nodiscard]] virtual const core::SemanticVersion& tool_version() const noexcept = 0;
    [[nodiscard]] virtual ToolExecutionOutcome execute(const ToolExecutionContext& context) const = 0;
};

// Invokes one implementation under the guarantees the platform owes every tool:
// cancellation is honoured before the call, a thrown exception becomes a technical
// error rather than an escape, an exceeded budget becomes a timeout, and successful
// outputs are validated against the declared output ports.
[[nodiscard]] ToolExecutionOutcome execute_tool(
    const ToolImplementation& implementation,
    const ToolExecutionContext& context);

}  // namespace nvision::workflow
