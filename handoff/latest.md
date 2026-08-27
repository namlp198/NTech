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
- Probed both OpenCV distributions under `D:\src\learn_opencv_all\Libraries` with a real
  CMake configure and both failed. `opencv480_CUDA` demands CUDA exactly 11.8 against an
  installed 11.2 and uses the removed `FindCUDA` module; the stock `opencv/build` is
  missing the `x64/vc16/bin` directory its own imported targets reference.
- OpenCV 4.14.0 was then installed at `D:\setups\opencv` and verified against the real
  build, which clears that blocker. `find_package` resolves it from the standard
  `OpenCV_DIR` environment variable and the runtime DLL is copied beside the native test
  executables.
- Added the M3 computed-image ownership contract in `5475413`, recorded as ADR-0007.
  `Image` owns its pixels and carries no acquisition metadata, `ImageSource` names which
  raster kind a view exposes, and `ToolExecutionContext::output_image_id` derives a stable
  identity for a produced image from the node and the output port.
- Made OpenCV a required dependency and added the interop substrate in `fe432d4`.
  `opencv_interop` borrows a view as a `cv::Mat` without copying and copies results back
  into an owned `Image`, with the pixel format declared by the producing tool because
  OpenCV records depth and channel count but not meaning.
  `nvision_public_contract_tests` scans every public include root for OpenCV and ONNX
  Runtime spellings, so the no-third-party-type rule is enforced rather than trusted.
- Implemented the whole Image Processing group in `43edcd8`: colour conversion, channel
  extraction, resize, normalise, CLAHE, blur, sharpen, adaptive threshold, colour range,
  morphology, distance transform, perspective warp, and affine transform, alongside the
  crop and threshold tools that landed with the substrate. `PixelFormat` gained
  `bayer_gr8`, `hsv8`, and `mono_float32`.
- Catalog coverage is 15 of 66 definitions, and every Image Processing definition is
  implemented.

## Current Status

M2 is complete and merged. M3 is in progress and green: the tool execution foundation, the
computed-image ownership contract, the OpenCV interop substrate, and the complete Image
Processing group are landed, and the native area boundary now matches the plan. Slices S2
through S6 remain. The branch has not been pushed and has not been merged into `develop`.

## Next Actions

1. Continue with slice S2, Location and Geometry, 11 tools. It needs native types the
   platform does not have yet: contour, detection list, feature set, and match set.
   Homography reuses `Transform2D`, which already guarantees an inverse exists.
2. Then S3 Measurement (11 tools, needs a measurement contract), S4 Clustering and
   Structure (6 tools, needs a point-grid model and result), and S5 overlay and evidence
   primitives.
3. Then S6 to close the M3 exit gate: refine the parameter sets that implementation
   showed to be wrong, prove the Pin Insert algorithms are representable without a Pin
   Insert engine class, and run the Linux native headless build.
4. Review and merge `codex/m3-tool-execution-foundation` into `develop`.
5. Keep the M0 asset provenance gap explicit and run the full repository check after each
   coherent behavior slice.
6. Decide whether to renormalize repository line endings as a separate commit.

## Warnings

- The catalog declares threshold modes `truncate` and `to-zero` on a tool whose output
  port is a mask. Both keep source intensity, so their result is an image rather than a
  mask, and the tool refuses them. The definition needs splitting or a version bump; it is
  recorded as an S6 item rather than silently widening what a mask means.
- `PixelFormat` gained one Bayer pattern, `bayer_gr8`, because that is the only one the
  catalog and the reference application need. The other three 8-bit patterns are absent on
  purpose and should be added with the tool or parameter that requires them.
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

- Full `scripts/check.ps1 -Configuration Release`: passed after every slice. The final
  run reported 0 native warnings under `/W4 /WX`, 12 CTests passed and 0 failed, 119 .NET
  tests passed and 0 failed, and a clean `dotnet format --verify-no-changes`.
- OpenCV check: with `OpenCV_DIR=D:\setups\opencv\build`, configure reported
  `Found OpenCV 4.14.0`, `nvision_vision` linked it, `opencv_world4140.dll` was copied
  into the native test output directory, and the gate stayed green.
- This is the first session in which the managed half of the gate actually ran.
- Negative check: removing the crop transform output made `nvision_tool_execution_tests`
  fail with `OUTPUT_CONTRACT_VIOLATION`, and restoring it returned the suite to green.
- Boundary check after the relocation: no header under `native/core`, `native/acquisition`,
  or `native/abi` includes anything from `nvision/workflow`, and no include of the old
  `nvision/core/` paths for the relocated files remains anywhere in `native/`.
