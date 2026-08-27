# Active Task

## Current Task

M3 — Generic OpenCV Vision Foundation — Tool Execution Foundation Slice

## Goal

Give the M2 tool contracts an execution plane. A resolved `ToolTypeDefinition` must be
bindable to a native implementation, invocable with typed port values, and testable in
isolation, without any public contract exposing an OpenCV type.

## Completed in Current Slice

- Added `nvision_workflow`: `PortValue`/`PortValueMap` typed carriers, `CancellationToken`
  and `CancellationSource`, `ExecutionBudget`, `ParameterBinding`, `ToolExecutionContext`,
  `ToolExecutionOutcome`, the `ToolImplementation` interface, and `execute_tool`.
- A context validates its inputs against the bound definition once. An undeclared key, a
  data-kind mismatch, an output key used as an input, and a missing required input are all
  rejected at construction.
- `ParameterBinding::resolve` fills declared parameters from the configuration or their
  defaults and drops keys the bound version does not declare, so a tool cannot read a value
  it never declared while the node keeps that value persisted.
- `execute_tool` honours cancellation before the call, converts a thrown exception into a
  technical error, converts an exceeded budget into a timeout, and validates successful
  outputs against the declared output ports. A non-success outcome carries no outputs.
- Added `nvision_vision` with the first real tool. Crop resolves a rectangular ROI against
  the input view, refuses a region authored in another coordinate space, and produces a
  subview plus the explicit view-to-source transform without copying pixels.
- `ToolImplementationRegistry` binds catalog identity to an implementation and reports a
  catalogued tool with no implementation as `not_implemented`, so M3 coverage stays
  measurable. 1 of 66 definitions is implemented.
- OpenCV is opt-in through `NVISION_ENABLE_OPENCV` and found by the standard CMake search.
  No build file holds a machine-specific path, and the dependency is PRIVATE to
  `nvision_vision` so no public header can expose an OpenCV type.
- Added the `nvision_tool_execution_tests` target. The native gate passes with 10 CTests.
- Relocated the M2 tool platform contracts from `nvision::core` to `nvision::workflow`:
  `parameter_schema`, `tool_definition`, `tool_registry`, `builtin_catalog`,
  `workflow_node`, `workflow_graph`, and `subworkflow`. Execution and the contracts it
  executes now live in one area, and the dependency is strictly one way. No behavior,
  contract, or test expectation changed, and the C ABI is untouched. Recorded as ADR-0006.

## Completed Earlier

- M1: generic IDs, semantic versions, port data kinds, coordinate-space-aware geometry and
  transforms, image views, resource references, immutable execution/inspection results,
  and separated technical status versus verdict. Merged by `d01e54c`.
- M2: versioned immutable `ToolTypeDefinition`, exact-resolution `ToolRegistry`,
  declarative parameter/property-editor schemas with acyclic conditional fields, persisted
  `WorkflowNode`/`NodeConfiguration` that survive an unresolved tool, typed acyclic
  `WorkflowGraph` with contract-checked binding, reusable subworkflows, and the initial
  66-tool generic catalog validating both reference graphs. Merged by `f1c0abd`.

## Next Steps

1. Implement the image-processing tool group against this foundation, golden-tested. The
   first OpenCV-backed slice decides how an operation that materializes a new image owns
   its buffer, which crop deliberately did not have to answer.
2. Continue through location/geometry, measurement, and clustering/point-grid groups.
3. Complete the external model/dependency license provenance audit before importing or
   redistributing reference assets; keep C ABI/gRPC exposure deferred until M8.

## Acceptance Criteria

- [x] A resolved tool definition can be bound to an implementation and executed.
- [x] An unimplemented definition is reported explicitly, never silently skipped.
- [x] Typed port values enforce the declared data kind at execution time.
- [x] A technical failure yields an execution error and never an OK/NG quality decision.
- [x] Cancellation and timeout are observable through the execution contract.
- [x] No public header or public contract exposes an OpenCV type.
- [x] The native and managed halves of `scripts/check.ps1` are both green.

## Verification Baseline

`scripts/check.ps1 -Configuration Release` is green on this machine: 10 native CTests,
119 .NET tests, and a clean `dotnet format --verify-no-changes`. The managed half resolves
.NET SDK 10.0.200 from the repository-local `artifacts/dotnet/` installation.

## Open Items

- No usable OpenCV is available on this machine, so `NVISION_ENABLE_OPENCV` defaults to
  OFF and no OpenCV-backed tool exists yet. Both distributions under
  `D:\src\learn_opencv_all\Libraries` were probed with a real CMake configure and both
  fail:
  - `opencv480_CUDA` requires CUDA toolkit exactly 11.8 and the machine has 11.2. Its
    config also uses the removed `FindCUDA` module, which CMake policy CMP0146 warns about
    and CMake 4 drops. A mandatory CUDA dependency is also wrong for a CPU-first portable
    Core.
  - `opencv/build` (4.8.0, vc16) is incomplete. Its `x64/vc16/bin` directory is gone
    because the DLLs were moved to `D:\src\learn_opencv_all\output\Release`, so the
    imported `opencv_world` target references files that do not exist.

  Sourcing a complete OpenCV 4.x package with documented provenance and license is a
  prerequisite for the next slice.

- External Wire Marker model and Pin Insert asset provenance/license evidence is still
  unresolved. Do not import or redistribute those assets until it is.
- Pin Insert golden/regression images are still unavailable, so it remains an
  architectural contract oracle rather than a parity suite.
- Repository line endings: committed blobs still contain CRLF while `.gitattributes`
  declares `* text=auto eol=lf`. A full Release build left the working tree clean, so this
  is currently latent rather than active. `git add --renormalize .` as its own commit
  remains the clean fix and still needs an explicit decision.
