# ADR-0001 — Platform-First Monorepo

**Status:** Accepted
**Date:** 2026-08-19

## Decision

NVision is developed as one platform monorepo containing applications, shared Engine capabilities, SDKs, plugins, schemas, tests, samples, tools, and documentation.

Customer inspection solutions are Projects, Jobs, Recipes, Models, and optional reusable plugins. They do not fork the platform.

## Consequences

- Shared contracts evolve in one repository.
- Vertical slices can validate Studio and Runtime together.
- Capability boundaries and architecture tests are mandatory to prevent a monolithic god application.
