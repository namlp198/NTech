# Active Task

## Current Task

M1 — Native Foundational Data Contracts — Complete on Feature Branch

## Goal

Create application-neutral portable C++20 contracts that can describe both Wire Marker
and Pin Insert workflows without embedding either application's vocabulary in Core.

## Completed in Current Slice

- Verified the merged N1 baseline on `develop`; the complete repository check passed.
- Created `codex/generic-vision-m1` from merge commit `bd46168`.
- Added typed canonical identifiers for Project, Workflow, Job, Recipe, execution
  session, resource, and coordinate-space identity.
- Added validated semantic versions with SemVer precedence independent of build metadata.
- Added the initial generic native `PortDataType` catalog required by both reference
  workflows.
- Added coordinate-space-aware point, line, circle, rectangle, polygon, and 2D
  homography contracts with finite/degenerate/mixed-space validation.
- Added a generic inspection outcome that rejects technical Error/Timeout/Cancelled
  states carrying valid-looking OK or NG verdicts.
- Added a dedicated native foundational-contract CTest proving generic reference-graph
  coverage and invalid-combination rejection.
- Added zero-copy native `ImageView` contracts for full frames, ROIs, and nested views.
  Views retain `FrameLease`, respect stride/padding, reject invalid ownership/bounds, and
  expose frame-specific coordinate spaces with explicit bidirectional transforms.
- Added unified immutable references for resource, model, template, calibration, and
  dictionary content. References bind stable ID and exact semantic version to canonical
  lowercase SHA-256 and positive expected content length without storing filesystem paths.
- Added immutable execution, decision, evidence, and inspection-result aggregates.
  Technical failure requires matching execution errors, remains Indeterminate, and cannot
  carry an OK/NG decision. Spatial evidence requires coordinate-space identity and all
  evidence IDs are unique within a finalized result.
- Added a dedicated execution-result CTest target; the full repository gate passes with
  4 native CTests and 119 .NET tests.

## Next Steps

1. Complete the external model/dependency license provenance audit before importing or
   redistributing any reference assets.
2. Perform the M2 Tool Definition/Catalog slice only after the next instruction.
3. Keep C ABI and gRPC exposure deferred until M8.

## Acceptance Criteria

- [x] Work occurs on a dedicated feature branch with the merged N1 baseline green.
- [x] Native generic types can represent the data flow of both reference workflows.
- [x] Geometry cannot exist without explicit coordinate-space identity.
- [x] Invalid/non-finite/degenerate geometry and mismatched transforms are rejected.
- [x] Technical failures cannot become OK or NG inspection verdicts.
- [x] Image views preserve native ownership and explicit transforms.
- [x] Resource/model/template references are immutable and versioned.
- [x] Result/evidence structures are immutable and reject invalid status combinations.
- [x] The complete M1 exit gate passes without application-specific Core types.
