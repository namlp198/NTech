# 00_CONTEXT

## Project

NTech NVision — Industrial Vision Intelligence Platform

## Purpose

Build a reusable industrial Machine Vision platform for authoring, publishing, executing, and operating automated visual inspections with traditional vision, AI, and industrial automation.

## Current State

- Working: P0 repository foundation, the complete managed P1 Workflow engine, the managed P2 Wire Marker headless inspection slice, N0 native foundation, N1 native Frame/File Camera/acquisition, the complete M1 generic native contract layer, and the complete M2 tool platform contract layer merged into `develop` by `f1c0abd`: versioned tool definitions, exact-resolution registry, declarative parameter schemas, persisted workflow nodes, typed acyclic graphs, reusable subworkflows, and the initial 66-tool generic catalog validated by both reference graphs.
- Working P2 path: FrameLease ownership → File Camera → AcquisitionService → compiled Crop/Classify/Compare/Decision Workflow → ONNX Runtime CPU → immutable InspectionResult → file image evidence + SQLite.
- Blocked: Pin Insert parity still lacks golden/regression images, and external Wire Marker model provenance/license evidence remains open before those assets can be redistributed. The previously recorded .NET SDK blocker is resolved: `scripts/check.ps1` resolves the pinned SDK 10.0.200 from the repository-local `artifacts/dotnet/` installation and the full gate is green on both planes.
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

M2 is complete and merged into `develop` by `f1c0abd`. It provides versioned immutable tool
definitions, exact registry resolution with explicit unresolved states, declarative
parameter and property-editor schemas, persisted workflow nodes whose configuration
survives an unresolved tool, typed acyclic graphs, reusable subworkflows, and the initial
66-tool generic catalog. Both reference graphs bind against that catalog with no
application-specific tool.

The current focus is M3, the generic OpenCV vision foundation. Its first slice is the tool
execution foundation: the native contract that makes a resolved tool definition invocable,
the implementation registry keyed by the same stable ID and exact version, typed port
values at execution time, and OpenCV dependency wiring kept behind the implementation
boundary. Concrete vision tools follow on top of it. Keep the M0 provenance gap explicit,
use the managed P2 path as a behavior oracle, and do not expand managed hot-path behavior.

## Verification Baseline

`scripts/check.ps1` builds and tests the native and managed planes, then verifies
formatting. The current baseline is 9 native CTests plus 119 .NET tests with a clean
`dotnet format --verify-no-changes`. The managed half requires .NET SDK 10.0.200 as pinned
by `global.json`; the scripts prefer the repository-local `artifacts/dotnet/` installation,
which is how that pin is satisfied on this machine.

## Notes for AI

- Read accepted ADRs before changing architecture.
- Do not infer real camera or PLC details before hardware selection.
- Update this file and `handoff/latest.md` when project state changes.
