# 00_CONTEXT

## Project

NTech NVision — Industrial Vision Intelligence Platform

## Purpose

Build a reusable industrial Machine Vision platform for authoring, publishing, executing, and operating automated visual inspections with traditional vision, AI, and industrial automation.

## Current State

- Working: P0 repository foundation, the complete managed P1 Workflow engine, the managed P2 Wire Marker headless inspection slice, N0 native foundation, N1 native Frame/File Camera/acquisition, the complete M1 generic native contract layer, the complete M2 tool platform contract layer merged into `develop` by `f1c0abd`, and the complete M3 generic OpenCV foundation merged by `d1faa1d`. The native catalog contains 66 product-neutral definitions, 43 of which execute through the M3 OpenCV implementation registry.
- Working P2 path: FrameLease ownership → File Camera → AcquisitionService → compiled Crop/Classify/Compare/Decision Workflow → ONNX Runtime CPU → immutable InspectionResult → file image evidence + SQLite.
- Blocked: Pin Insert parity still lacks golden/regression images, and external Wire Marker model provenance/license evidence remains open before those assets can be redistributed. Two previously recorded blockers are resolved: `scripts/check.ps1` resolves the pinned .NET SDK 10.0.200 from the repository-local `artifacts/dotnet/` installation, and OpenCV 4.14.0 is installed and verified, discovered through the standard `OpenCV_DIR` environment variable.
- Not started: native model/ONNX and Workflow/inspection migration, the Engine Host implementation, ProjectFormat/Studio authoring, Runtime Service, PLC integration, real camera plugins, and production-trained inspection models.
- Known contract hardening before later milestones depend on the current catalog: an executable collection/multi-value model, typed plural outputs, exact non-identity transform tests, self-describing evidence, richer structured results, full subworkflow parameter assignability, and truthful cancellation/timeout characteristics. The durable guidance is `docs/implementation/17_GENERIC_TOOL_CATALOG_EVOLUTION_GUIDE.md`.

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
- Native area ownership follows ADR-0006: `core` holds the data vocabulary, `workflow`
  holds the tool platform and execution, `vision` holds the image tool implementations,
  and the dependency runs one way only.
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

M3, the generic OpenCV vision foundation, is complete. `nvision_workflow` makes a resolved
tool definition invocable through typed port values, a validated execution context,
cancellation and budget, and an implementation registry keyed by definition identity.
`nvision_vision` implements 43 of the 66 catalog definitions: the whole Image Processing,
Location and Geometry, and Measurement groups, the clustering and point-grid group, and the
overlay and evidence primitives. Both reference grid flows run end to end on those tools
alone, with no application-specific tool in the catalog. A definition without an
implementation binds as not_implemented, so coverage stays measurable, and the two
deliberate gaps are asserted rather than left silent.

An independent M2/M3 generalization review confirmed that the architecture is
platform-first but that current validation is still driven by the two reference
applications. The review does not replace or invalidate the M3 implementation baseline;
it adds mandatory cross-milestone gates before M4/M5/M8 build on contracts that are not
yet executable or portable enough. These gates are recorded in
`docs/implementation/17_GENERIC_TOOL_CATALOG_EVOLUTION_GUIDE.md` and linked from the
authoritative execution plan.

The next focus is M4, the generic model and recognition foundation: `.nvmodel` integrity,
a model registry with instance-owned sessions, the native ONNX Runtime CPU backend, text
detection and recognition pre- and post-processing, CTC decoding, and the Recognition and
Text tools. Keep the M0 provenance gap explicit, use the managed P2 path as a behavior
oracle, and do not expand managed hot-path behavior. M4 must first select the typed
collection/multi-value representation used by text candidates, recognition crops,
barcode lists, and consensus inputs.

## Verification Baseline

`scripts/check.ps1` builds and tests the native and managed planes, then verifies
formatting. The current baseline is 13 native CTests plus 119 .NET tests with a clean
`dotnet format --verify-no-changes`. The managed half requires .NET SDK 10.0.200 as pinned
by `global.json`; the scripts prefer the repository-local `artifacts/dotnet/` installation,
which is how that pin is satisfied on this machine. The native vision library requires
OpenCV 4.x, located through the standard `OpenCV_DIR` environment variable.

## Notes for AI

- Read accepted ADRs before changing architecture.
- Do not infer real camera or PLC details before hardware selection.
- Update this file and `handoff/latest.md` when project state changes.
