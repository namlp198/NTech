# Active Task

## Current Task

P2 — Wire Marker Headless Vertical Slice — Complete

## Goal

Run and persist one complete deterministic image inspection through the real reference ONNX backend without UI or hardware dependencies.

## Completed in Current P2 Slice

- Immutable image/frame/pixel-format contracts and reference-counted `FrameLease` ownership.
- Replaceable camera contract, deterministic PGM File Camera, and hardened AcquisitionService timeout/cancellation.
- Directory-backed `.nvmodel` manifest loader with SHA-256 artifact validation, immutable model registry, and Recipe-level slot resolution.
- Replaceable ONNX Runtime 1.28 CPU backend running a real deterministic ONNX graph.
- Versioned reference preprocessing and classification postprocessing.
- Crop, Classify, Compare, and Decision nodes executed through P1 validation, compilation, and sequential runtime.
- Immutable InspectionResult validation with strict quality-versus-technical status semantics.
- File image evidence and SQLite result persistence/reconstruction.
- TYPE_01/TYPE_04 fixtures and configurations, golden integration tests, repeated ownership coverage, and CLI demo script.

## Next Steps

1. Review P3 ProjectFormat and Studio MVP scope.
2. Promote Wire Marker source/configuration into the editable `.nvision` ProjectFormat and immutable `.nvpkg` publish path.

## Acceptance Criteria

- [x] `TYPE_01 + TYPE_01` produces `OK`.
- [x] `TYPE_04 + TYPE_01` produces `NG / WRONG_TYPE`.
- [x] Backend failure produces persisted `ERROR`, never ordinary `NG`.
- [x] Result and exact Project/Job/Recipe/Model/plugin/backend versions are persisted.
- [x] SQLite reconstructs the canonical result and file evidence has checksums.
- [x] Repeated inspection and explicit FrameLease reference-count tests pass.
- [x] Architecture, unit, integration, and format checks pass.
