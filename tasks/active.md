# Active Task

## Current Task

M1 — Native Foundational Data Contracts — In Progress

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

## Next Steps

1. Add native image-view contracts that retain explicit source image and coordinate-space
   ownership without copying full frame buffers.
2. Add versioned resource/model/template references and integrity metadata.
3. Add immutable execution, result, evidence, overlay, measurement, and defect contracts.
4. Complete the external model/dependency license provenance audit before importing or
   redistributing any reference assets.
5. Keep C ABI and gRPC exposure deferred until the internal M1 contracts pass their exit
   gate and are stable enough for boundary design.

## Acceptance Criteria

- [x] Work occurs on a dedicated feature branch with the merged N1 baseline green.
- [x] Native generic types can represent the data flow of both reference workflows.
- [x] Geometry cannot exist without explicit coordinate-space identity.
- [x] Invalid/non-finite/degenerate geometry and mismatched transforms are rejected.
- [x] Technical failures cannot become OK or NG inspection verdicts.
- [ ] Image views preserve native ownership and explicit transforms.
- [ ] Resource/model/template references are immutable and versioned.
- [ ] Result/evidence structures are immutable and reject invalid status combinations.
- [ ] The complete M1 exit gate passes without application-specific Core types.
