# Active Task

## Current Task

M2 — Tool Definition, Catalog, and Parameter Schema — In Progress

## Goal

Create an application-neutral native Tool Catalog whose stable versioned definitions,
schemas, and resolution behavior can describe both reference workflows without coupling
persisted identity to Studio display structure.

## Completed in Current Slice

- Created `codex/generic-vision-m2` from the completed M1 feature branch.
- Added strong Tool Type, Plugin, and Port Key identities.
- Added immutable versioned `ToolTypeDefinition` contracts with typed ports,
  hierarchical category metadata, tags, descriptions, built-in/plugin origin, and
  execution lifetime/device/cancellation/debug characteristics.
- Added a native `ToolRegistry` keyed only by stable Tool Type ID and exact semantic
  version. Exact resolution distinguishes unknown IDs from unsupported versions and
  preserves the unresolved request identity.
- Proved category paths are mutable catalog metadata rather than persisted registry
  identity.
- Added `nvision_tool_catalog_tests`; the full repository gate passes with 5 native
  CTests and 119 .NET tests.

## Next Steps

1. Add the declarative parameter/property-editor schema for scalar, enum, array, geometry,
   ROI, resource, model, calibration, and conditional fields.
2. Add reusable subworkflow and preserved unresolved-node contracts, then validate both
   reference graphs against the native catalog.
3. Complete the external model/dependency license provenance audit before importing or
   redistributing reference assets; keep C ABI/gRPC exposure deferred until M8.

## Acceptance Criteria

- [x] Work occurs on a dedicated M2 feature branch with the M1 baseline green.
- [x] Tool definitions have stable ID plus exact semantic version.
- [x] Category metadata can change without changing registry identity.
- [x] Typed ports, built-in/plugin origin, and execution characteristics are validated.
- [x] Registry enumeration and exact resolution preserve explicit unresolved states.
- [ ] Declarative parameter/property-editor schemas cover all required generic kinds.
- [ ] Reusable subworkflow and unresolved-node configuration contracts are preserved.
- [ ] The initial generic catalog validates both reference graphs at contract level.
