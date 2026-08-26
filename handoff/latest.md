# Latest Handoff

## Date

2026-08-26

## Branch

`codex/generic-vision-m2-subworkflow`, created from `develop` at `007cf92`

## Completed

- Confirmed the M2 foundation is merged into `develop` by `d01e54c` and that local
  `develop` is now in sync with `origin/develop`, which the previous handoff had not yet
  recorded.
- Added the remaining M2 contract slice in `00f86c6`.
- Added `WorkflowNodeId` and immutable `WorkflowNode`. A node persists only the stable
  tool type ID and its exact semantic version, never catalog display structure.
- Added `NodeConfiguration`. Parameter assignments keep their authored order and values
  and duplicate keys are rejected at construction.
- Added `WorkflowDiagnostic` with stable result codes, warning/error severity, and
  optional node, parameter, or port scope.
- Added `bind_workflow_node`. An unknown tool or unsupported version is reported through
  `UNKNOWN_TOOL_TYPE` or `UNSUPPORTED_TOOL_VERSION` and the configuration survives intact.
  A resolved node reports `PARAMETER_TYPE_MISMATCH` and `MISSING_REQUIRED_PARAMETER` as
  errors, and preserves an undeclared key as an `UNKNOWN_PARAMETER` warning rather than
  deleting it.
- Added `SubworkflowDefinition`, which publishes a reusable graph through an ordinary
  versioned tool interface using port mappings and parameter promotions. An exposed input
  may feed several internal ports; an exposed output has exactly one source.
- Structural violations fail at construction: no nodes, duplicate node IDs, self
  containment, an unmapped interface port, an output with several sources, a node port
  mapped twice, and promotions that are not one to one.
- Added `bind_subworkflow`. Registry-dependent checks report `MISSING_MAPPED_PORT`,
  `PORT_TYPE_MISMATCH`, `MISSING_PROMOTED_PARAMETER`, and
  `PROMOTED_PARAMETER_TYPE_MISMATCH`, warn on an `UNPROMOTED_PARAMETER`, and stay silent
  for an unresolved node so one missing plugin cannot mask the real cause.
- Added the `nvision_workflow_contract_tests` target covering unresolved-node
  preservation, schema drift, structural rejection, and binding diagnostics.

## Current Status

M2 contracts are complete on the feature branch and not yet merged. The branch has not
been pushed. The next M2 work is enumerating the initial generic built-in catalog tree and
validating both reference graphs at contract level.

## Next Actions

1. Enumerate the initial generic built-in Tool Catalog tree from plan Section 8.
2. Validate the Wire Marker and Pin Insert reference graphs against that catalog using the
   node, subworkflow, and parameter contracts.
3. Keep the M0 asset provenance gap explicit and run the full repository check after each
   coherent behavior slice once the pinned .NET SDK is available.

## Warnings

- The full `scripts/check.ps1` gate could not run. `global.json` pins .NET SDK 10.0.200
  and only 9.0.312 is installed, so the managed build, the .NET tests, and
  `dotnet format --verify-no-changes` did not execute. This slice touches only native C++
  and CMake files, so the managed plane is unaffected by the change itself, but the
  managed half of the gate is unverified in this session.
- Pin Insert golden/regression images remain unavailable, so it is currently an
  architectural contract oracle rather than a parity suite.
- ONNX Runtime and OpenCV distributions contain license files, but no license/provenance
  evidence was found beside the external Wire Marker project/models or the provided Pin
  Insert project. Do not import or redistribute those assets until provenance is resolved.
- The new contracts are internal C++ APIs. Do not expose them through C ABI/gRPC until the
  boundary work reaches M8.

## Verification

- Native Release build with `/W4 /WX`: passed with 0 warnings.
- CTest: 7 passed, 0 failed, including the new `nvision_workflow_contract_tests`.
- Full `scripts/check.ps1 -NoRestore`: not completed; it failed at the managed build step
  because the pinned .NET SDK 10.0.200 is missing from this machine.
