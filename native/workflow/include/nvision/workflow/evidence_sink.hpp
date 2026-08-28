#pragma once

#include "nvision/core/resource_reference.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace nvision::workflow {

// Where evidence a tool produced is kept.
//
// A tool hands over the bytes it wants recorded and receives the integrity that identifies
// them. It never learns whether they went to a file, a database or a memory buffer, and it
// never computes the digest itself: the store knows what it actually wrote, so the store
// is the only honest source of that answer.
class EvidenceSink {
public:
    EvidenceSink() = default;
    EvidenceSink(const EvidenceSink&) = delete;
    EvidenceSink& operator=(const EvidenceSink&) = delete;
    EvidenceSink(EvidenceSink&&) = delete;
    EvidenceSink& operator=(EvidenceSink&&) = delete;
    virtual ~EvidenceSink() = default;

    [[nodiscard]] virtual core::ResourceIntegrity store(
        const std::string& storage_key,
        std::span<const std::uint8_t> content) = 0;
};

}  // namespace nvision::workflow
