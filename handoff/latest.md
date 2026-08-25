# Latest Handoff

## Date

2026-08-25

## Branch

`codex/generic-vision-m1`

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

## Current Status

M1 is in progress. The first foundational-contract slice is implemented and native tests
pass in `c07440b`. Image views, versioned resource references, and immutable result/
evidence contracts remain before the M1 exit gate is complete.

## Next Actions

1. Implement a native image-view contract tied to retained image ownership and explicit
   source/target coordinate spaces.
2. Implement generic immutable resource/model/template references with version and
   checksum metadata.
3. Implement immutable execution/result/evidence aggregates using the status/verdict
   invariant already established.
4. Run the full repository check after each coherent behavior slice.

## Warnings

- Pin Insert golden/regression images remain unavailable, so it is currently an
  architectural contract oracle rather than a parity suite.
- ONNX Runtime and OpenCV distributions contain license files, but no license/provenance
  evidence was found beside the external Wire Marker project/models or the provided Pin
  Insert project. Do not import or redistribute those assets until provenance is resolved.
- The new contracts are internal C++ APIs. Do not expose them through C ABI/gRPC until M1
  stabilizes and the boundary work reaches M8.

## Verification

- Baseline full `scripts/check.ps1`: passed before M1 changes.
- Native Release build with `/W4 /WX`: passed after M1 changes.
- CTest: 3 passed, 0 failed.
- Full post-change `scripts/check.ps1`: passed; 3 native CTests and 119 .NET tests passed,
  with 0 build warnings and formatting verification clean.
