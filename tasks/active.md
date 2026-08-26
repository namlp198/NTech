# Active Task

## Current Task

M2 — Tool Definition, Catalog, and Parameter Schema — Complete on the Feature Branch

## Goal

Create an application-neutral native Tool Catalog whose stable versioned definitions,
schemas, and resolution behavior can describe both reference workflows without coupling
persisted identity to Studio display structure.

## Completed in Current Slice

- Added `WorkflowConnection` and `WorkflowGraph`. Topology is validated without a registry:
  unique node IDs, known endpoints, unique connections, no self edge, and no cycle.
- Added `bind_workflow_graph`. Contract-dependent checks report an undeclared port, a data
  type mismatch, a second source on a single-source input, and a required input with no
  source. A caller may declare externally fed inputs, which is how a subworkflow satisfies
  the internal inputs its interface maps.
- Reworked `SubworkflowDefinition` to own a `WorkflowGraph` instead of a node list, and
  collapsed `SubworkflowBinding` into `WorkflowGraphBinding`.
- Added `register_builtin_tools`, enumerating the 66 tool definitions of plan Section 8
  across all seven generic categories.
- Validated the three reference graphs of plan Section 12 against that one catalog. All
  bind fully resolved with no diagnostics.
- Added a test that no catalog definition carries reference application vocabulary, and
  tests that orientation search, matcher backend, capture role, and the learned grid
  resource are parameters rather than separate tools.
- Added `contour_list` and `text_result_list` port kinds; the singular forms existed while
  the other collection kinds did not.
- Added the `nvision_builtin_catalog_tests` and `nvision_reference_graph_tests` targets.
  The native gate passes with 9 CTests and a clean `/W4 /WX` Release build.

## Completed Earlier in M2

- Versioned immutable `ToolTypeDefinition` contracts with typed ports, hierarchical
  category metadata, built-in/plugin origin, and execution characteristics.
- A native `ToolRegistry` keyed only by stable Tool Type ID and exact semantic version.
- Declarative parameter and property-editor schemas, including acyclic conditional fields.
- Persisted `WorkflowNode` and `NodeConfiguration`, whose values survive an unresolved tool,
  and the reusable subworkflow contract.
- Merge `d01e54c` brought the completed M1 chain and the M2 foundation into `develop`.

## Next Steps

1. Review and merge `codex/generic-vision-m2-subworkflow` into `develop`, then close M2 in
   the plan progress record.
2. Start M3, the generic OpenCV vision foundation, implementing the catalog definitions
   behind the contracts and refining their parameter sets against real behavior.
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
- [x] The initial generic catalog validates both reference graphs at contract level.

## Blocked

- The full `scripts/check.ps1` gate cannot run in the current environment. `global.json`
  requires .NET SDK 10.0.200 and only 9.0.312 is installed, so the managed build, the
  .NET tests, and `dotnet format --verify-no-changes` do not execute. This work changes
  only native C++ and CMake files, and the native half of the gate is green.
