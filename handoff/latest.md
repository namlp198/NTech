# Latest Handoff

## Date

2026-08-26

## Branch

`codex/generic-vision-m2`

## Completed

- Confirmed N1 is merged into `develop` by `bd46168` and verified the clean baseline with
  `scripts/check.ps1` before starting M1.
- Added strong application-neutral native IDs and validated semantic-version precedence.
- Added the initial generic native port data kinds needed by text inspection and point-
  grid inspection reference graphs.
- Added coordinate-space-aware point, line, circle, rectangle, polygon, and homography
  contracts. Invalid finite values, degenerate shapes, singular transforms, and mixed
  coordinate spaces fail at construction or application boundaries.
- Added separate `ExecutionStatus` and `InspectionVerdict` contracts plus validation that
  prevents Error, Timeout, or Cancelled execution from carrying OK/NG.
- Added `nvision_foundational_contracts_tests`; all 3 native CTests pass with warnings as
  errors.
- Added zero-copy `ImageView` in `0c71340`. Full-frame, ROI, and nested views retain the
  Frame lease, expose stride-aware read-only rows, reject incomplete/anonymous frames and
  invalid bounds, and carry frame-specific source/local spaces with explicit transforms.
- Added unified versioned resource references in `4339211`. Resource/model/template/
  calibration/dictionary references carry stable IDs, exact versions, kinds, canonical
  SHA-256 digests, and positive content lengths without filesystem paths.
- Added immutable execution/result/evidence aggregates in `e8b7073`. The aggregate locks
  technical Error/Timeout/Cancelled to Indeterminate, forbids quality decisions on failed
  execution, requires matching stable error codes, and enforces unique coordinate-aware
  spatial evidence.
- Started M2 in `a2f1a25` with immutable versioned Tool Definitions and a native registry.
  Definitions cover typed ports, hierarchical metadata, origin, and execution
  characteristics. Resolution is exact by ID/version and preserves unknown versus
  unsupported-version states independently of category metadata.
- Added declarative parameter/property-editor schemas in `b143c6a`. The contracts cover
  enum, typed arrays, geometry, ROI, and versioned resource/model/calibration references,
  with type-safe defaults, UI-neutral editor metadata, and acyclic conditional fields.

## Current Status

M1 remains complete, and M2 is in progress on `codex/generic-vision-m2`. Tool
Definition/Catalog (`a2f1a25`) and declarative parameter schemas (`b143c6a`) are
implemented and green. The feature branch has not been merged or pushed.

## Next Actions

1. Add reusable subworkflow and unresolved-node preservation contracts, then validate the
   two generic reference graphs.
2. Enumerate the initial generic built-in Tool Catalog tree.
3. Keep the M0 asset provenance gap explicit and run the full repository check after each
   coherent behavior slice.

## Warnings

- Pin Insert golden/regression images remain unavailable, so it is currently an
  architectural contract oracle rather than a parity suite.
- ONNX Runtime and OpenCV distributions contain license files, but no license/provenance
  evidence was found beside the external Wire Marker project/models or the provided Pin
  Insert project. Do not import or redistribute those assets until provenance is resolved.
- The new contracts are internal C++ APIs. Do not expose them through C ABI/gRPC until the
  boundary work reaches M8.

## Verification

- Completed M1 baseline full `scripts/check.ps1`: passed before M2 changes.
- Native Release build with `/W4 /WX`: passed after the current M2 changes.
- CTest: 6 passed, 0 failed.
- Full post-change `scripts/check.ps1`: passed; 6 native CTests and 119 .NET tests passed,
  with 0 build warnings and formatting verification clean.
