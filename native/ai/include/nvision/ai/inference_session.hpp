#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace nvision::ai {

// A model that could not be loaded, or a run that the backend refused.
//
// It is a distinct type so a caller can tell a backend failure from an ordinary logic
// error. A tool that lets one escape still ends the run as a technical failure, because
// execute_tool turns any exception into one rather than letting it reach the engine.
class InferenceError final : public std::runtime_error {
public:
    explicit InferenceError(const std::string& message);
};

// One tensor a session takes or produces: its name and its declared shape.
//
// A dimension the model leaves free, such as a batch size, is reported as -1 rather than
// guessed. A caller that needs an exact size has to say what it wants, and a caller that
// does not can see which dimensions are its to choose.
class TensorSpec final {
public:
    TensorSpec(std::string name, std::vector<std::int64_t> dimensions);

    [[nodiscard]] const std::string& name() const noexcept;
    [[nodiscard]] const std::vector<std::int64_t>& dimensions() const noexcept;
    [[nodiscard]] bool is_fully_defined() const noexcept;
    // The number of elements a fully defined shape holds. Throws when a dimension is free,
    // because there is no honest answer to give.
    [[nodiscard]] std::size_t element_count() const;

private:
    std::string name_;
    std::vector<std::int64_t> dimensions_;
};

// An ONNX Runtime session over one model, owned by the instance that created it.
//
// The runtime never appears in this header: no ONNX Runtime type, no include, and no
// handle a caller could mistake for one. That is the boundary nvision_public_contract_tests
// enforces, and it is what keeps the backend replaceable.
//
// A session is movable but not copyable, because two owners of one model session would
// have no agreed lifetime. Running is const and the backend is safe to call concurrently
// on one session; loading is not, and is expected to happen once.
class InferenceSession final {
public:
    // Loads from bytes rather than from a path, because a model arrives inside a versioned
    // package whose integrity has already been checked. Reading the file again would be
    // checking one thing and running another.
    [[nodiscard]] static InferenceSession from_model_bytes(
        std::span<const std::uint8_t> model,
        std::string session_name = "nvision");

    InferenceSession(const InferenceSession&) = delete;
    InferenceSession& operator=(const InferenceSession&) = delete;
    InferenceSession(InferenceSession&&) noexcept;
    InferenceSession& operator=(InferenceSession&&) noexcept;
    ~InferenceSession();

    [[nodiscard]] const std::vector<TensorSpec>& inputs() const noexcept;
    [[nodiscard]] const std::vector<TensorSpec>& outputs() const noexcept;

    // Runs a single float input and returns the single float output.
    //
    // Deliberately narrow: it covers what one classifier or one recognizer head needs, and
    // refuses anything else rather than pretending to be the general case. Multi-input
    // models, non-float tensors and batched runs arrive with M4, when there is a real
    // consumer to shape them against.
    [[nodiscard]] std::vector<float> run(
        std::span<const float> input,
        const std::vector<std::int64_t>& input_shape) const;

private:
    class State;

    explicit InferenceSession(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> state_;
};

// The ONNX Runtime version the native plane is linked against, for diagnostics and for the
// capability metadata M4 has to report.
[[nodiscard]] std::string backend_version();

}  // namespace nvision::ai
