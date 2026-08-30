#include "nvision/ai/inference_session.hpp"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <numeric>
#include <utility>

namespace nvision::ai {

namespace {

// One environment per process. ONNX Runtime expects it to outlive every session, and a
// function-local static gives that without a global whose destruction order is unclear.
Ort::Env& environment() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "nvision");
    return env;
}

std::vector<TensorSpec> specs_of(
    const Ort::Session& session,
    const bool inputs,
    Ort::AllocatorWithDefaultOptions& allocator) {
    const std::size_t count = inputs ? session.GetInputCount() : session.GetOutputCount();
    std::vector<TensorSpec> specs;
    specs.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
        const Ort::AllocatedStringPtr name =
            inputs ? session.GetInputNameAllocated(index, allocator)
                   : session.GetOutputNameAllocated(index, allocator);
        const Ort::TypeInfo info = inputs ? session.GetInputTypeInfo(index)
                                          : session.GetOutputTypeInfo(index);
        specs.emplace_back(
            std::string(name.get()),
            info.GetTensorTypeAndShapeInfo().GetShape());
    }
    return specs;
}

}  // namespace

InferenceError::InferenceError(const std::string& message) : std::runtime_error(message) {}

TensorSpec::TensorSpec(std::string name, std::vector<std::int64_t> dimensions)
    : name_(std::move(name)), dimensions_(std::move(dimensions)) {}

const std::string& TensorSpec::name() const noexcept {
    return name_;
}

const std::vector<std::int64_t>& TensorSpec::dimensions() const noexcept {
    return dimensions_;
}

bool TensorSpec::is_fully_defined() const noexcept {
    return std::all_of(
        dimensions_.begin(), dimensions_.end(), [](const std::int64_t dimension) {
            return dimension > 0;
        });
}

std::size_t TensorSpec::element_count() const {
    if (!is_fully_defined()) {
        throw InferenceError("tensor " + name_ + " leaves a dimension free, so it has no fixed size");
    }
    return std::accumulate(
        dimensions_.begin(),
        dimensions_.end(),
        std::size_t{1U},
        [](const std::size_t total, const std::int64_t dimension) {
            return total * static_cast<std::size_t>(dimension);
        });
}

// Holds everything the runtime owns. Declaring it here rather than in the header is what
// keeps every ONNX Runtime type out of the public contract.
class InferenceSession::State final {
public:
    State(std::span<const std::uint8_t> model, const std::string& session_name)
        : options_(),
          session_(
              environment(),
              model.data(),
              model.size(),
              configure(options_, session_name)) {
        Ort::AllocatorWithDefaultOptions allocator;
        inputs_ = specs_of(session_, true, allocator);
        outputs_ = specs_of(session_, false, allocator);
    }

    [[nodiscard]] const std::vector<TensorSpec>& inputs() const noexcept {
        return inputs_;
    }

    [[nodiscard]] const std::vector<TensorSpec>& outputs() const noexcept {
        return outputs_;
    }

    [[nodiscard]] std::vector<float> run(
        std::span<const float> input,
        const std::vector<std::int64_t>& input_shape) {
        const char* const input_name = inputs_.front().name().c_str();
        const char* const output_name = outputs_.front().name().c_str();

        const Ort::MemoryInfo memory =
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        // ONNX Runtime borrows the caller's buffer for the length of the run rather than
        // copying it, which is why run() takes a span and does not outlive the call.
        Ort::Value tensor = Ort::Value::CreateTensor<float>(
            memory,
            const_cast<float*>(input.data()),
            input.size(),
            input_shape.data(),
            input_shape.size());

        std::vector<Ort::Value> produced = session_.Run(
            Ort::RunOptions{nullptr}, &input_name, &tensor, 1U, &output_name, 1U);
        if (produced.size() != 1U || !produced.front().IsTensor()) {
            throw InferenceError("the model did not produce a single output tensor");
        }

        const auto shape = produced.front().GetTensorTypeAndShapeInfo();
        if (shape.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            throw InferenceError("the model produced an output that is not 32-bit float");
        }
        const float* values = produced.front().GetTensorData<float>();
        return std::vector<float>(values, values + shape.GetElementCount());
    }

private:
    static Ort::SessionOptions& configure(
        Ort::SessionOptions& options,
        const std::string& session_name) {
        options.SetLogId(session_name.c_str());
        // The plan puts the production backend on CPU, and a session that quietly picked a
        // device would make results depend on the machine that ran them.
        options.SetIntraOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        return options;
    }

    Ort::SessionOptions options_;
    Ort::Session session_;
    std::vector<TensorSpec> inputs_;
    std::vector<TensorSpec> outputs_;
};

InferenceSession::InferenceSession(std::unique_ptr<State> state) noexcept
    : state_(std::move(state)) {}

InferenceSession::InferenceSession(InferenceSession&&) noexcept = default;
InferenceSession& InferenceSession::operator=(InferenceSession&&) noexcept = default;
InferenceSession::~InferenceSession() = default;

InferenceSession InferenceSession::from_model_bytes(
    std::span<const std::uint8_t> model,
    std::string session_name) {
    if (model.empty()) {
        throw InferenceError("a model package contained no bytes to load");
    }
    try {
        return InferenceSession(std::make_unique<State>(model, session_name));
    } catch (const Ort::Exception& error) {
        // The backend's own message is the only one that says what was actually wrong with
        // the model, so it is carried through rather than replaced with a generic one.
        throw InferenceError(std::string("the model could not be loaded: ") + error.what());
    }
}

const std::vector<TensorSpec>& InferenceSession::inputs() const noexcept {
    return state_->inputs();
}

const std::vector<TensorSpec>& InferenceSession::outputs() const noexcept {
    return state_->outputs();
}

std::vector<float> InferenceSession::run(
    std::span<const float> input,
    const std::vector<std::int64_t>& input_shape) const {
    if (state_->inputs().size() != 1U || state_->outputs().size() != 1U) {
        throw InferenceError(
            "this session runs a single input to a single output, and the model declares "
            "neither");
    }
    if (input_shape.empty()) {
        throw InferenceError("an input tensor needs a shape");
    }
    const auto declared = std::accumulate(
        input_shape.begin(),
        input_shape.end(),
        std::size_t{1U},
        [](const std::size_t total, const std::int64_t dimension) {
            return dimension > 0 ? total * static_cast<std::size_t>(dimension) : total;
        });
    if (std::any_of(
            input_shape.begin(),
            input_shape.end(),
            [](const std::int64_t dimension) { return dimension <= 0; }) ||
        declared != input.size()) {
        throw InferenceError("the input shape does not describe the values supplied");
    }
    try {
        return state_->run(input, input_shape);
    } catch (const Ort::Exception& error) {
        throw InferenceError(std::string("inference failed: ") + error.what());
    }
}

std::string backend_version() {
    return std::string(OrtGetApiBase()->GetVersionString());
}

}  // namespace nvision::ai
