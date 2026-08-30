// Proves the native plane really links and runs ONNX Runtime, rather than only finding it.
//
// The fixture is the synthetic identity model already in the repository: 146 bytes, one
// Identity node from "input" to "scores". That makes the assertion exact rather than
// vague. What comes out must be what went in, so a wrong tensor layout, a truncated
// buffer, or an output read at the wrong offset all fail here instead of surviving as a
// plausible-looking number.

#include "nvision/ai/inference_session.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                           \
        if (!(condition)) {        \
            std::abort();          \
        }                          \
    } while (false)

namespace {

using nvision::ai::InferenceError;
using nvision::ai::InferenceSession;
using nvision::ai::TensorSpec;

template <typename Action>
bool rejects_inference(Action action) {
    try {
        action();
    } catch (const InferenceError&) {
        return true;
    }
    return false;
}

std::vector<std::uint8_t> decode_base64(const std::string& encoded) {
    static constexpr std::string_view alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<std::uint8_t> decoded;
    std::uint32_t accumulator = 0U;
    int bits = 0;
    for (const char character : encoded) {
        if (character == '=') {
            break;
        }
        const std::size_t index = alphabet.find(character);
        if (index == std::string_view::npos) {
            // Whitespace and line breaks are formatting, not content.
            continue;
        }
        accumulator = (accumulator << 6U) | static_cast<std::uint32_t>(index);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            decoded.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xFFU));
        }
    }
    return decoded;
}

std::vector<std::uint8_t> reference_model() {
    const std::filesystem::path path =
        std::filesystem::path(NVISION_TEST_MODEL_DIR) / "wire-marker-1.0.0.nvmodel" / "model" /
        "model.onnx.base64";
    std::ifstream stream(path, std::ios::binary);
    NVISION_REQUIRE(stream.is_open());
    std::ostringstream contents;
    contents << stream.rdbuf();
    return decode_base64(contents.str());
}

// A shape the model will accept: whatever it declares, with any free dimension taken as
// one. Reading the shape instead of assuming it means the test still says something true
// if the fixture is ever regenerated at a different size.
std::vector<std::int64_t> concrete_shape(const TensorSpec& spec) {
    std::vector<std::int64_t> shape = spec.dimensions();
    for (std::int64_t& dimension : shape) {
        if (dimension <= 0) {
            dimension = 1;
        }
    }
    return shape;
}

std::size_t element_count(const std::vector<std::int64_t>& shape) {
    std::size_t total = 1U;
    for (const std::int64_t dimension : shape) {
        total *= static_cast<std::size_t>(dimension);
    }
    return total;
}

void the_native_plane_loads_and_runs_a_real_model() {
    const std::vector<std::uint8_t> model = reference_model();
    NVISION_REQUIRE(!model.empty());

    const InferenceSession session =
        InferenceSession::from_model_bytes(model, "nvision.test.identity");

    NVISION_REQUIRE(session.inputs().size() == 1U);
    NVISION_REQUIRE(session.outputs().size() == 1U);
    NVISION_REQUIRE(session.inputs().front().name() == "input");
    NVISION_REQUIRE(session.outputs().front().name() == "scores");

    const std::vector<std::int64_t> shape = concrete_shape(session.inputs().front());
    const std::size_t count = element_count(shape);
    NVISION_REQUIRE(count > 0U);

    std::vector<float> values(count);
    for (std::size_t index = 0U; index < count; ++index) {
        // Distinct, exactly representable values, so a reordering is visible.
        values[index] = static_cast<float>(index) * 0.5F;
    }

    const std::vector<float> scores = session.run(values, shape);
    NVISION_REQUIRE(scores.size() == values.size());
    for (std::size_t index = 0U; index < scores.size(); ++index) {
        // Identity, so equality is exact rather than approximate. Anything else means the
        // buffer was not the one the model read.
        NVISION_REQUIRE(scores[index] == values[index]);
    }

    // The same session run twice gives the same answer, which is the determinism the
    // inspection contract depends on.
    const std::vector<float> again = session.run(values, shape);
    NVISION_REQUIRE(again == scores);
}

void a_session_reports_what_it_cannot_do_instead_of_guessing() {
    NVISION_REQUIRE(rejects_inference([] {
        (void)InferenceSession::from_model_bytes({});
    }));
    NVISION_REQUIRE(rejects_inference([] {
        const std::vector<std::uint8_t> nonsense(64U, 0x7FU);
        (void)InferenceSession::from_model_bytes(nonsense);
    }));

    const std::vector<std::uint8_t> model = reference_model();
    const InferenceSession session = InferenceSession::from_model_bytes(model);
    const std::vector<std::int64_t> shape = concrete_shape(session.inputs().front());

    // A shape that does not describe the values is refused rather than reinterpreted.
    const std::vector<float> values(element_count(shape));
    NVISION_REQUIRE(rejects_inference([&session, &values] {
        (void)session.run(values, {});
    }));
    NVISION_REQUIRE(rejects_inference([&session, &values, &shape] {
        std::vector<std::int64_t> doubled = shape;
        doubled.front() *= 2;
        (void)session.run(values, doubled);
    }));
}

void the_backend_reports_the_version_it_is_linked_against() {
    const std::string version = nvision::ai::backend_version();
    NVISION_REQUIRE(!version.empty());
    // Not pinned to an exact string: the point is that the answer comes from the linked
    // runtime rather than from a constant this repository maintains by hand.
    NVISION_REQUIRE(version.find('.') != std::string::npos);
}

void a_tensor_spec_admits_when_it_has_no_fixed_size() {
    const TensorSpec fixed("input", {1, 64});
    NVISION_REQUIRE(fixed.is_fully_defined());
    NVISION_REQUIRE(fixed.element_count() == 64U);

    const TensorSpec batched("input", {-1, 64});
    NVISION_REQUIRE(!batched.is_fully_defined());
    bool refused = false;
    try {
        (void)batched.element_count();
    } catch (const InferenceError&) {
        refused = true;
    }
    NVISION_REQUIRE(refused);
}

}  // namespace

int main() {
    the_native_plane_loads_and_runs_a_real_model();
    a_session_reports_what_it_cannot_do_instead_of_guessing();
    the_backend_reports_the_version_it_is_linked_against();
    a_tensor_spec_admits_when_it_has_no_fixed_size();
    return EXIT_SUCCESS;
}
