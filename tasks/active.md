# Active Task

## Current Task

M2 — Tool Definition, Catalog, and Parameter Schema — In Progress, Contracts Complete

## Goal

Create an application-neutral native Tool Catalog whose stable versioned definitions,
schemas, and resolution behavior can describe both reference workflows without coupling
persisted identity to Studio display structure.

## Completed in Current Slice

- Created `codex/generic-vision-m2-subworkflow` from `develop` at `007cf92`.
- Added `WorkflowNodeId` and immutable `WorkflowNode` instances that persist only stable
  tool type ID plus exact semantic version.
- Added `NodeConfiguration`, whose parameter assignments keep their authored order and
  values and reject duplicate keys.
- Added `WorkflowDiagnostic` with stable result codes, warning/error severity, and
  optional node, parameter, and port scope.
- Added `bind_workflow_node`. An unknown tool or an unsupported version is reported and
  never repaired, and the full configuration survives. A resolved node reports
  type-incompatible values and missing required parameters as errors, and preserves an
  undeclared parameter key as a warning instead of deleting it.
- Added `SubworkflowDefinition`. A reusable graph is published through an ordinary
  versioned tool interface with port mappings and parameter promotions.
- Structural violations fail at construction: no nodes, duplicate node IDs, a subworkflow
  that contains itself, an unmapped interface port, an output mapped to more than one
  source, a node port mapped twice, and promotions that are not one to one.
- Added `bind_subworkflow`. Port and promotion checks that need registry resolution
  surface as diagnostics, and stay silent for an unresolved node so that one missing
  plugin cannot mask the real cause.
- Added the `nvision_workflow_contract_tests` target; the native gate passes with 7 CTests
  and a clean `/W4 /WX` Release build.

## Completed Earlier in M2

- Strong Tool Type, Plugin, and Port Key identities.
- Immutable versioned `ToolTypeDefinition` contracts with typed ports, hierarchical
  category metadata, tags, descriptions, built-in/plugin origin, and execution
  lifetime/device/cancellation/debug characteristics.
- A native `ToolRegistry` keyed only by stable Tool Type ID and exact semantic version.
- Declarative `ParameterValue`, `ParameterType`, `ParameterDefinition`, and Property
  Editor metadata contracts, including acyclic conditional visibility.
- Merge `d01e54c` brought the completed M1 chain and the M2 foundation into `develop`.

## Next Steps

1. Enumerate the initial generic built-in Tool Catalog tree from plan Section 8.
2. Validate both reference graphs against that catalog at contract level using the node,
   subworkflow, and parameter contracts.
3. Complete the external model/dependency license provenance audit before importing or
   redistributing reference assets; keep C ABI/gRPC exposure deferred until M8.

## Acceptance Criteria

- [x] Work occurred on a dedicated M2 feature branch and the green foundation is merged.
- [x] Tool definitions have stable ID plus exact semantic version.
- [x] Category metadata can change without changing registry identity.
- [x] Typed ports, built-in/plugin origin, and execution characteristics are validated.
- [x] Registry enumeration and exact resolution preserve explicit unresolved states.
- [x] Declarative parameter/property-editor schemas cover all required generic kinds.
- [x] Reusable subworkflow and unresolved-node configuration contracts are preserved.
- [ ] The initial generic catalog validates both reference graphs at contract level.

## Blocked

- The full `scripts/check.ps1` gate cannot run in the current environment. `global.json`
  requires .NET SDK 10.0.200 and only 9.0.312 is installed, so the managed build, the
  .NET tests, and `dotnet format --verify-no-changes` do not execute. This slice changes
  only native C++ and CMake files, and the native half of the gate is green.
