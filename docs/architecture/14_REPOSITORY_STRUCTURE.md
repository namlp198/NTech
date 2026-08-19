# 14 — NVision Repository Structure

**Status:** Accepted
**Date:** 2026-08-19

## Decision

NVision uses a capability-oriented monorepo. The canonical root is:

```text
apps/
engine/
sdk/
plugins/
schemas/
tests/
samples/
tools/
docs/
```

This layout follows Section 28 of the Platform Foundation. It represents the product ecosystem before framework-specific layering.

## Ownership

- `apps` owns executable composition and presentation.
- `engine` owns reusable platform behavior and stable internal contracts.
- `sdk` owns public embedding/control surfaces.
- `plugins` owns replaceable vendor and backend implementations.
- `schemas` owns versioned serialized compatibility contracts.
- `tests` owns verification organized by purpose.
- `samples` owns executable reference applications and regression evidence.
- `tools` owns engineering utilities, not production domain behavior.
- `docs` owns shared product and engineering knowledge.

## Physical Project Rule

A `.csproj` is created only when it has a defined responsibility, an explicit dependency boundary, a consumer, and a verification path. Future capabilities may be represented by documentation without empty assemblies.

## Dependency Direction

```text
Core <- Domain/Contracts <- Engines <- Application <- Executable Apps
                     ^          ^
                     |          |
              Plugin Abstractions <- Concrete Plugins
```

Concrete plugins depend inward. Core and Engines never depend on a concrete plugin.

## Prohibited Root Layout

Do not reorganize the repository into a generic application-only root such as `src/core`, `src/infrastructure`, and `src/ui`.

Clean Architecture layering may be used inside each capability, but it must not hide NVision's platform boundaries.

## Operational Folders

The following folders support repository operation and do not change the product architecture:

```text
tasks/
handoff/
decisions/
scripts/
outputs/
artifacts/
```

Actual ADR documents live in `docs/decisions`. `decisions/0000_INDEX.md` is the repository workflow index.
