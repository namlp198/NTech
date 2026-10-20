#include "nvision/workflow/evidence_sink.hpp"

#include <stdexcept>
#include <utility>

namespace nvision::workflow {

StoredEvidence::StoredEvidence(core::ResourceIntegrity integrity, std::string reference)
    : integrity_(std::move(integrity)), reference_(std::move(reference)) {
    if (reference_.empty()) {
        // An artifact nobody can locate again is not stored evidence, whatever was written.
        throw std::invalid_argument("stored evidence must report where it was kept");
    }
}

const core::ResourceIntegrity& StoredEvidence::integrity() const noexcept {
    return integrity_;
}

const std::string& StoredEvidence::reference() const noexcept {
    return reference_;
}

}  // namespace nvision::workflow
