# Latest Handoff

## Date

2026-08-27

## Branch

`codex/m3-tool-execution-foundation`, created from `develop` at `f1c0abd`

## Completed

- Synced the context documents to reality in `1b8e992`. M2 was already merged into
  `develop` by `f1c0abd`, but `tasks/active.md`, `docs/00_CONTEXT.md`, and both the
  progress record and immediate-next-action sections of the execution plan still described
  it as complete on a feature branch awaiting merge.
- Corrected the recorded .NET SDK blocker in the same commit. The previous handoff reported
  that only SDK 9.0.312 was installed and that the managed half of the gate could not run.
  The repository already carries a local SDK 10.0.200 under `artifacts/dotnet/`, and every
  script prefers it over the machine-wide `dotnet`, so the full gate runs here.
- Added the M3 tool execution foundation in `861ba0f`.
- `nvision_workflow` is a new native library owning execution: `PortValue`/`PortValueMap`,
  `CancellationToken`/`CancellationSource`, `ExecutionBudget`, `ParameterBinding`,
  `ToolExecutionContext`, `ToolExecutionOutcome`, `ToolImplementation`, `execute_tool`, and
  `ToolImplementationRegistry`.
- `ToolExecutionContext` validates its inputs against the bound definition once, so an
  implementation never rechecks that a port is declared, is an input rather than an output,
  carries the declared data kind, or is present when the contract marks it required.
- `ParameterBinding::resolve` fills declared parameters from the node configuration or the
  declared default and drops keys the bound version does not declare. The node still
  persists those keys; the tool simply never receives a value it did not declare.
- `execute_tool` honours cancellation before invoking, turns a thrown exception into a
  technical error instead of letting it unwind into the engine, turns an exceeded budget
  into a timeout, and validates successful outputs against the declared output ports. A
  non-success outcome cannot carry outputs at all, so a failed tool can never hand a
  downstream node a value that looks valid.
- `nvision_vision` is a new native library for the generic image tools and carries the
  first real one. Crop resolves a rectangular ROI against the input view, refuses a region
  authored in another coordinate space, rejects a sub-pixel or out-of-bounds region, and
  produces a subview plus the explicit view-to-source transform without copying pixels.
- A catalogued definition with no implementation binds as `not_implemented` rather than as
  a missing tool, so remaining M3 coverage is measurable. 1 of the 66 definitions is
  implemented.
- OpenCV is opt-in through `NVISION_ENABLE_OPENCV` and located by the standard CMake
  search. No build file holds a machine-specific path, and the dependency is PRIVATE to
  `nvision_vision` so no public header can expose an OpenCV type.
- Relocated the M2 tool platform contracts from `nvision::core` to `nvision::workflow` in
  `d73f4d8`, recorded as ADR-0006. `parameter_schema`, `tool_definition`, `tool_registry`,
  `builtin_catalog`, `workflow_node`, `workflow_graph`, and `subworkflow` moved so that
  execution and the contracts it executes share one area, as Section 13 of the plan
  requires. The change is mechanical, the resulting dependency is strictly one way, and no
  core, acquisition, or ABI header includes a workflow header.
- Probed both OpenCV distributions on this machine with a real CMake configure. Both fail,
  so no OpenCV-backed tool can be written yet. Details are in `tasks/active.md`.

## Current Status

M2 is complete and merged. M3 has started: the tool execution foundation is landed and
green, the first catalog definition executes through it end to end, and the native area
boundary now matches the plan. The branch has not been pushed and has not been merged into
`develop`.

## Next Actions

1. Review and merge `codex/m3-tool-execution-foundation` into `develop`.
2. Source a complete OpenCV 4.x package with documented provenance and license before the
   next slice. Neither distribution on this machine configures: `opencv480_CUDA` demands
   CUDA exactly 11.8 against an installed 11.2 and relies on the removed `FindCUDA`
   module, and the stock `opencv/build` is missing the `x64/vc16/bin` directory its own
   imported targets reference. A mandatory CUDA dependency is also wrong for a CPU-first
   portable Core. This is the one decision blocking the next slice.
3. Implement the image-processing tool group against the foundation, golden-tested. That
   slice must decide how an operation which materializes a new image owns its buffer;
   crop deliberately did not have to answer that because it only produces another view.
4. Continue through location/geometry, measurement, and clustering/point-grid groups.
5. Keep the M0 asset provenance gap explicit and run the full repository check after each
   coherent behavior slice.
6. Decide whether to renormalize repository line endings as a separate commit.

## Warnings

- Crop is the only implemented tool and it produces a view, never a new buffer. The first
  tool that materializes an image will expose an ownership question the current
  `PortValue` image payload has not had to answer, because it can only hold an `ImageView`
  that retains an existing frame lease.
- `PortValue` covers the port data kinds that have a native representation today. Kinds
  such as `feature_set`, `match_set`, `detection_list`, and `point_grid_model` have no
  factory yet and gain one with the milestone that introduces their native type.
- The execution budget is enforced after the fact by `execute_tool`, which measures elapsed
  time and reports a timeout. It does not interrupt a running tool. A tool that must stop
  early has to observe the cancellation token itself.
- The catalog parameter sets remain contract-level minimums. Implementing a tool may
  require a version bump on its definition; crop needed none.
- Pin Insert golden/regression images remain unavailable, so it is currently an
  architectural contract oracle rather than a parity suite.
- ONNX Runtime and OpenCV distributions contain license files, but no license/provenance
  evidence was found beside the external Wire Marker project/models or the provided Pin
  Insert project. Do not import or redistribute those assets until provenance is resolved.
- Repository line endings are still inconsistent in the committed blobs while
  `.gitattributes` declares `* text=auto eol=lf`. A full Release build left the working
  tree clean this session, so the problem is currently latent. `git add --renormalize .` as
  its own commit remains the clean fix and still needs an explicit decision.
- The new contracts are internal C++ APIs. Do not expose them through C ABI/gRPC until the
  boundary work reaches M8.

## Verification

- Full `scripts/check.ps1 -Configuration Release`: passed. Native Release build with
  `/W4 /WX` reported 0 warnings, CTest reported 10 passed and 0 failed, the .NET suites
  reported 119 passed and 0 failed, and `dotnet format --verify-no-changes` was clean.
- This is the first session in which the managed half of the gate actually ran.
- Negative check: removing the crop transform output made `nvision_tool_execution_tests`
  fail with `OUTPUT_CONTRACT_VIOLATION`, and restoring it returned the suite to green.
