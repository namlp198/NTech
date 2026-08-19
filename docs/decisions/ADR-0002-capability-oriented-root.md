# ADR-0002 — Capability-Oriented Repository Root

**Status:** Accepted
**Date:** 2026-08-19

## Decision

Use `apps`, `engine`, `sdk`, `plugins`, `schemas`, `tests`, `samples`, `tools`, and `docs` as the canonical repository root capabilities.

Do not use a generic `src/core`, `src/infrastructure`, and `src/ui` layout as the primary repository taxonomy.

## Consequences

- Repository navigation mirrors the NVision product ecosystem.
- Clean Architecture remains available within each capability.
- New projects require explicit capability ownership.
