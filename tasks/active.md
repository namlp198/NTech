# Active Task

## Current Task

M2 — Tool Definition, Catalog, and Parameter Schema — In Progress, Foundation Merged

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
- Added declarative `ParameterValue`, `ParameterType`, `ParameterDefinition`, and Property
  Editor metadata contracts for scalar, enum, recursively typed array, geometry, ROI, and
  versioned resource/model/calibration fields.
- Integrated parameters into immutable Tool Definitions. Finalization rejects duplicate
  keys, incompatible defaults/editor controls, missing or type-incompatible condition
  controllers, self-dependencies, and conditional visibility cycles.
- Added dedicated Tool Catalog and Parameter Schema test targets; the full repository gate
  passes with 6 native CTests and 119 .NET tests.
- Merged the completed M1 chain and current M2 foundation into `develop` with merge commit
  `d01e54c`; the full post-merge repository gate remains green.

## Next Steps

1. Add reusable subworkflow and preserved unresolved-node contracts, then validate both
   reference graphs against the native catalog.
2. Enumerate the initial generic built-in tool tree with the completed definition/schema
   contracts.
3. Complete the external model/dependency license provenance audit before importing or
   redistributing reference assets; keep C ABI/gRPC exposure deferred until M8.

## Acceptance Criteria

- [x] Work occurred on a dedicated M2 feature branch and the green foundation is merged.
- [x] Tool definitions have stable ID plus exact semantic version.
- [x] Category metadata can change without changing registry identity.
- [x] Typed ports, built-in/plugin origin, and execution characteristics are validated.
- [x] Registry enumeration and exact resolution preserve explicit unresolved states.
- [x] Declarative parameter/property-editor schemas cover all required generic kinds.
- [ ] Reusable subworkflow and unresolved-node configuration contracts are preserved.
- [ ] The initial generic catalog validates both reference graphs at contract level.
