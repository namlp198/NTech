# Latest Handoff

## Date

2026-08-26

## Branch

`codex/generic-vision-m2-subworkflow`, created from `develop` at `007cf92`

## Completed

- Added the remaining M2 contract slice in `00f86c6`: `WorkflowNodeId`, immutable
  `WorkflowNode`, `NodeConfiguration`, `WorkflowDiagnostic`, `bind_workflow_node`, and
  `SubworkflowDefinition`. An unresolved tool never causes stored configuration to be
  removed or rewritten, and an undeclared parameter key is preserved as a warning.
- Added typed connections and graphs in `bc58e5a`. `WorkflowGraph` validates topology
  without a registry: unique node IDs, known endpoints, unique connections, no self edge,
  and no cycle. `bind_workflow_graph` adds the contract-dependent checks and reports
  `MISSING_CONNECTED_PORT`, `CONNECTION_TYPE_MISMATCH`, `PORT_ALREADY_CONNECTED`, and
  `MISSING_REQUIRED_INPUT`.
- A caller may declare inputs fed from outside the graph. A subworkflow uses that for its
  interface mappings, so an internal required input is not falsely reported.
- `SubworkflowDefinition` now owns a `WorkflowGraph` rather than a node list. The previous
  shape had no edges, so a reference graph could not be expressed. `SubworkflowBinding`
  collapsed into `WorkflowGraphBinding` because both carried the same data.
- Added the initial generic Tool Catalog in `a777314`: 66 built-in versioned definitions
  across the seven categories of plan Section 8.
- Validated all three reference graphs of plan Section 12 against that one catalog. Each
  binds fully resolved with zero diagnostics. Verified the check is not vacuous by
  mutating one edge to an incompatible type and confirming the test fails.
- Added a test that no catalog definition carries reference application vocabulary, and
  tests that orientation search, matcher backend, capture role, and the learned grid
  resource are parameters rather than separate tools.
- Added `contour_list` and `text_result_list` port kinds. The singular forms already
  existed while the other collection kinds did not, so contour extraction and text split
  had no typed output.

## Current Status

M2 is complete on the feature branch and every Section 15 exit gate for the milestone is
met. All eight acceptance criteria in `tasks/active.md` are checked. The branch has not
been pushed and has not been merged into `develop`.

## Next Actions

1. Review and merge `codex/generic-vision-m2-subworkflow` into `develop`.
2. Start M3, the generic OpenCV vision foundation, implementing the catalog definitions
   behind their contracts and refining parameter sets against real behavior.
3. Keep the M0 asset provenance gap explicit and run the full repository check after each
   coherent behavior slice once the pinned .NET SDK is available.

## Warnings

- The full `scripts/check.ps1` gate could not run. `global.json` pins .NET SDK 10.0.200
  and only 9.0.312 is installed, so the managed build, the .NET tests, and
  `dotnet format --verify-no-changes` did not execute. This work touches only native C++
  and CMake files, so the managed plane is unaffected by the change itself, but the
  managed half of the gate is unverified in this session.
- The catalog parameter sets are contract-level minimums chosen to cover the reference
  graphs. M3 and M4 will refine them alongside the implementations, which may require
  version bumps on individual tool definitions.
- Pin Insert golden/regression images remain unavailable, so it is currently an
  architectural contract oracle rather than a parity suite.
- ONNX Runtime and OpenCV distributions contain license files, but no license/provenance
  evidence was found beside the external Wire Marker project/models or the provided Pin
  Insert project. Do not import or redistribute those assets until provenance is resolved.
- The new contracts are internal C++ APIs. Do not expose them through C ABI/gRPC until the
  boundary work reaches M8.

## Verification

- Native Release build with `/W4 /WX`: passed with 0 warnings.
- CTest: 9 passed, 0 failed, including `nvision_workflow_contract_tests`,
  `nvision_builtin_catalog_tests`, and `nvision_reference_graph_tests`.
- Negative check: mutating one reference graph edge to an incompatible data type failed
  the suite with `CONNECTION_TYPE_MISMATCH`, then the revert returned it to green.
- Full `scripts/check.ps1 -NoRestore`: not completed; it failed at the managed build step
  because the pinned .NET SDK 10.0.200 is missing from this machine.
