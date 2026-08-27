# Active Task

## Current Task

M3 — Generic OpenCV Vision Foundation — Tool Execution Foundation Slice

## Goal

Give the M2 tool contracts an execution plane. A resolved `ToolTypeDefinition` must be
bindable to a native implementation, invocable with typed port values, and testable in
isolation, without any public contract exposing an OpenCV type.

## Current Slice Scope

- A native tool execution contract that receives typed inputs, bound parameters, and a
  cancellation/timeout context, and returns typed outputs plus technical execution status.
- A tool implementation registry that binds a definition's stable ID and exact semantic
  version to an implementation, and reports an unimplemented definition explicitly.
- A typed port value carrier that matches the M1 port data kinds and enforces kind
  agreement at execution time.
- OpenCV dependency wiring in CMake for the native vision target, kept behind the
  implementation boundary so no public header includes OpenCV.

Concrete image-processing, geometry, measurement, recognition, and clustering tools are
implemented on top of this foundation in later M3 slices.

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

1. Land the tool execution foundation with its own native tests.
2. Implement the image-processing tool group against that foundation, golden-tested.
3. Continue through location/geometry, measurement, and clustering/point-grid groups.
4. Complete the external model/dependency license provenance audit before importing or
   redistributing reference assets; keep C ABI/gRPC exposure deferred until M8.

## Acceptance Criteria

- [ ] A resolved tool definition can be bound to an implementation and executed.
- [ ] An unimplemented definition is reported explicitly, never silently skipped.
- [ ] Typed port values enforce the declared data kind at execution time.
- [ ] A technical failure yields an execution error and never an OK/NG quality decision.
- [ ] Cancellation and timeout are observable through the execution contract.
- [ ] No public header or public contract exposes an OpenCV type.
- [ ] The native and managed halves of `scripts/check.ps1` are both green.

## Verification Baseline

`scripts/check.ps1 -Configuration Release` is green on this machine: 9 native CTests,
119 .NET tests, and a clean `dotnet format --verify-no-changes`. The managed half resolves
.NET SDK 10.0.200 from the repository-local `artifacts/dotnet/` installation.

## Open Items

- External Wire Marker model and Pin Insert asset provenance/license evidence is still
  unresolved. Do not import or redistribute those assets until it is.
- Pin Insert golden/regression images are still unavailable, so it remains an
  architectural contract oracle rather than a parity suite.
- Repository line endings: committed blobs still contain CRLF while `.gitattributes`
  declares `* text=auto eol=lf`. A full Release build left the working tree clean, so this
  is currently latent rather than active. `git add --renormalize .` as its own commit
  remains the clean fix and still needs an explicit decision.
