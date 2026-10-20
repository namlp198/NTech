#pragma once

#include "nvision/core/resource_reference.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace nvision::workflow {

// What a store reports after it has written an artifact: the integrity that identifies the
// bytes, and the reference that finds them again. Both come from the store because the
// store is the only thing that knows what it actually wrote and where it put it.
class StoredEvidence final {
public:
    StoredEvidence(core::ResourceIntegrity integrity, std::string reference);

    [[nodiscard]] const core::ResourceIntegrity& integrity() const noexcept;
    [[nodiscard]] const std::string& reference() const noexcept;

private:
    core::ResourceIntegrity integrity_;
    std::string reference_;
};

// Where evidence a tool produced is kept.
//
// A tool hands over the bytes it wants recorded and receives back what identifies and
// locates them. It never learns whether they went to a file, a database or a memory
// buffer, and it never computes the digest itself: the store knows what it actually wrote,
// so the store is the only honest source of that answer. The storage key is what the tool
// asked for; the returned reference is where the artifact really is.
class EvidenceSink {
public:
    EvidenceSink() = default;
    EvidenceSink(const EvidenceSink&) = delete;
    EvidenceSink& operator=(const EvidenceSink&) = delete;
    EvidenceSink(EvidenceSink&&) = delete;
    EvidenceSink& operator=(EvidenceSink&&) = delete;
    virtual ~EvidenceSink() = default;

    [[nodiscard]] virtual StoredEvidence store(
        const std::string& storage_key,
        std::span<const std::uint8_t> content) = 0;
};

}  // namespace nvision::workflow
