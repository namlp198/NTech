# ADR-0003 — Modular Monolith First

**Status:** Accepted
**Date:** 2026-08-19

## Decision

Start with in-process Engine modules and plugins while preserving explicit contracts and dependency boundaries.

Move a component out of process only for demonstrated isolation, stability, security, or deployment needs.

## Consequences

- MVP debugging and deployment remain simple.
- Camera and AI paths avoid premature IPC overhead.
- Native-risk plugins may later move to isolated hosts without redesigning core contracts.
