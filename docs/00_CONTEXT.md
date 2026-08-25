# 00_CONTEXT

## Project

NTech NVision — Industrial Vision Intelligence Platform

## Purpose

Build a reusable industrial Machine Vision platform for authoring, publishing, executing, and operating automated visual inspections with traditional vision, AI, and industrial automation.

## Current State

- Working: P0 repository foundation, the complete managed P1 Workflow engine, the managed P2 Wire Marker headless inspection slice, N0 native foundation, N1 native Frame/File Camera/acquisition, the complete M1 generic native contract layer, and the first M2 versioned Tool Definition/Catalog slice on `codex/generic-vision-m2`.
- Working P2 path: FrameLease ownership → File Camera → AcquisitionService → compiled Crop/Classify/Compare/Decision Workflow → ONNX Runtime CPU → immutable InspectionResult → file image evidence + SQLite.
- Blocked: Nothing in the current M2 contract slice. Pin Insert parity still lacks golden/regression images, and external Wire Marker model provenance/license evidence remains open before those assets can be redistributed.
- Not started: native model/ONNX and Workflow/inspection migration, the Engine Host implementation, ProjectFormat/Studio authoring, Runtime Service, PLC integration, real camera plugins, and production-trained inspection models.

## Architecture Summary

NVision is a capability-oriented modular monolith. Executable products live in `apps`; reusable headless behavior lives in `engine`; vendor and backend variability lives in `plugins`; durable contracts live in `schemas`. Studio and Runtime are separate applications using the same Engine.

The approved implementation boundary is native-first. Canonical Core and Engine
execution are portable C++20. Studio, Runtime UI, Runtime supervision, persistence,
authoring, and business-oriented products use C#/.NET 10. Embedded clients use the
versioned C ABI through P/Invoke; production Runtime supervises a native Engine Host
through gRPC. Large images remain in native/shared memory. See ADR-0005 and
`docs/architecture/15_NATIVE_ENGINE_ARCHITECTURE.md`.

## Important Constraints

- Follow `docs/architecture/14_REPOSITORY_STRUCTURE.md`.
- Domain and Engine cannot depend on WPF or vendor SDKs.
- New canonical Core/Engine behavior cannot be implemented only in managed code.
- OpenCV and ONNX Runtime production execution belong to the native C++ plane.
- Technical inspection failures never become valid-looking OK/NG results.
- File Camera, PLC Simulator, and ONNX Runtime precede real hardware.

## Current Development Focus

M2 Tool Definition/Catalog is active on the feature branch. Its first slice provides
versioned immutable definitions with typed ports, hierarchical display metadata,
built-in/plugin origin, execution characteristics, exact registry resolution, and explicit
unknown-tool versus unsupported-version states. Category metadata is never registry
identity. Continue with the generic parameter/property-editor schema while keeping the M0
provenance gap explicit. Use the managed P2 path as a behavior oracle; do not expand
managed hot-path behavior.

## Verification Baseline

`scripts/check.ps1` builds and tests the native and managed planes, then verifies
formatting. The current baseline is 5 native CTests plus 119 .NET tests.

## Notes for AI

- Read accepted ADRs before changing architecture.
- Do not infer real camera or PLC details before hardware selection.
- Update this file and `handoff/latest.md` when project state changes.
