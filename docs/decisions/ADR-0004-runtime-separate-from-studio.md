# ADR-0004 — Runtime Separate from Studio

**Status:** Accepted
**Date:** 2026-08-19

## Decision

Studio and Runtime are separate applications. Runtime is headless-first and owns production readiness, inspection execution, recovery, and traceability. Runtime UI is a client, not the production authority.

## Consequences

- Production continues when Studio or Runtime UI is closed.
- Studio testing must use the same Engine as Runtime.
- Deployment and Recipe changes pass Runtime validation.
