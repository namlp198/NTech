// Guards the architectural rule that no public NVision contract may expose a
// third-party vision or AI type. The rule is easy to state and easy to break by adding
// one include to the wrong header, so it is checked rather than trusted.

#include "nvision/vision/builtin_tool_implementations.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                           \
        if (!(condition)) {        \
            std::abort();          \
        }                          \
    } while (false)

namespace {

// Every directory below native/ that a consumer of the native plane may include from.
constexpr std::string_view public_include_roots[] = {
    "abi/include",
    "acquisition/include",
    "core/include",
    "vision/include",
    "workflow/include",
};

// Spellings that only appear when a third-party header or type has reached a contract.
constexpr std::string_view forbidden_spellings[] = {
    "opencv",
    "cv::",
    "onnxruntime",
    "Ort::",
};

std::string read(const std::filesystem::path& file) {
    std::ifstream stream(file, std::ios::binary);
    std::ostringstream contents;
    contents << stream.rdbuf();
    return contents.str();
}

void public_headers_expose_no_third_party_vision_or_ai_type() {
    const std::filesystem::path native_root(NVISION_NATIVE_SOURCE_DIR);
    std::size_t scanned = 0U;

    for (const std::string_view root : public_include_roots) {
        const std::filesystem::path include_root = native_root / root;
        NVISION_REQUIRE(std::filesystem::is_directory(include_root));

        for (const auto& entry : std::filesystem::recursive_directory_iterator(include_root)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::filesystem::path& file = entry.path();
            if (file.extension() != ".hpp" && file.extension() != ".h") {
                continue;
            }

            const std::string contents = read(file);
            for (const std::string_view spelling : forbidden_spellings) {
                NVISION_REQUIRE(contents.find(spelling) == std::string::npos);
            }
            ++scanned;
        }
    }

    // A scan that silently found nothing would pass without proving anything.
    NVISION_REQUIRE(scanned >= 15U);
}

void the_scan_would_notice_a_violation() {
    // Confirms the check itself works, so a green result means the headers are clean
    // rather than that the comparison never fires.
    const std::string offending = "#include <opencv2/core.hpp>";
    bool detected = false;
    for (const std::string_view spelling : forbidden_spellings) {
        detected = detected || offending.find(spelling) != std::string::npos;
    }
    NVISION_REQUIRE(detected);
}

void the_vision_library_is_usable_without_any_third_party_header() {
    // This translation unit includes only NVision public headers, so the fact that it
    // compiles and can build a registry is itself part of the contract.
    const auto registry = nvision::vision::create_vision_implementation_registry();
    NVISION_REQUIRE(registry.size() >= 1U);
}

}  // namespace

int main() {
    public_headers_expose_no_third_party_vision_or_ai_type();
    the_scan_would_notice_a_violation();
    the_vision_library_is_usable_without_any_third_party_header();
    return 0;
}
