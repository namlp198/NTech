# ADR-0003 — Modular Monolith First

**Status:** Accepted
**Date:** 2026-08-19

**Amended by:** ADR-0005 on 2026-08-20

## Decision

Keep one capability-oriented product and repository with explicit contracts and
dependency boundaries. The managed P0–P2 reference may remain in-process during
migration.

ADR-0005 establishes the demonstrated production boundary: canonical Core/Engine is
native C++, embedded clients use its C ABI, and production Runtime supervises the native
Engine Host out of process through gRPC.

## Consequences

- Capability ownership remains modular-monolith-first rather than microservice-first.
- The same native engine library backs embedded and hosted modes.
- Process separation is restricted to the approved Engine Host and later demonstrated
  isolation needs; internal capabilities do not become distributed services by default.
