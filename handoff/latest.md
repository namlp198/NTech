# Latest Handoff

## Date

2026-08-28

## Branch

`develop`. The M3 work was done on `codex/m3-tool-execution-foundation`, created from
`develop` at `f1c0abd`, and merged back by `d1faa1d`.

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
- Added the location and geometry data contracts in `9f2c0d3`: `Contour`, `Measurement`,
  `Detection`, `Feature`, `FeatureSet`, `FeatureMatch`, and `MatchSet`. A contour is
  deliberately not a polygon, because a single-pixel or collinear region is a real result a
  detector must be able to report. A measurement carries a unit, and one in image units
  must name the coordinate space it was taken in. A match set carries both feature sets, so
  a homography estimate recovers the point pairs without being handed the images again.
- Implemented all 11 Location and Geometry tools in `ab9b17c`. Coordinate space is checked
  wherever two inputs must agree, contour parent indices are remapped through the area
  filter rather than passed through stale, point assignment consumes each candidate once,
  and algorithmic failure is reported as a recoverable vision error while a contract
  problem is reported as a configuration error.
- Implemented 10 of the 11 Measurement tools in `a4c2108`. Lengths carry the coordinate
  space they were measured in, counts and quality figures name none, and comparing two
  rasters requires a shared pixel format and extent.
- Implemented the clustering and point-grid group in `4b72161`, together with the grid
  deviation tool held back from the measurement slice. `PointGridModel` records where
  features are expected and nothing about what they mean; tolerances belong to the recipe
  that inspects against it. K-Means fixes OpenCV's thread-local generator, DBSCAN is
  implemented directly and expands in input order, and both sorting paths are stable, so a
  repeated run produces identical results.
- Point grid inspect is deliberately not implemented. It receives its model as a versioned
  resource rather than through a port, a considered catalog decision that keeps training
  and inspection as separate runs joined by a recipe binding. That binding arrives with M5,
  and a tool that could only ever report an unbound resource would be worse than none. Its
  `point_grid_result` contract is unwritten for the same reason. The coverage test asserts
  the gap.
- The tool execution suite no longer names a specific unimplemented tool. It asks the
  registry which definitions are still pending, so filling in the catalog no longer breaks
  a test that is not about the tool being added.
- Added the overlay and evidence primitives in `462b558`. `Overlay` carries shapes, styles
  and labels as data, so the same overlay renders into evidence, a Studio canvas or a
  Runtime preview and no result depends on a drawing having happened. `EvidenceSink` is the
  seam evidence output needed: a tool hands over bytes and receives the integrity that
  identifies them, never learning where they are kept and never computing the digest,
  because the store is the only thing that knows what it actually wrote.
- Evidence Output reports what the raster really is, using the ADR-0007 provenance: a view
  over a captured frame becomes source evidence, a computed raster derived evidence, and an
  annotated one an overlay. With no sink attached it reports the fact rather than
  discarding what it made.
- Closed the exit gate in `6df1505`. Two catalog definitions promised what their contract
  cannot deliver, which only became visible once the tools existed: threshold declared
  truncate and to-zero on a tool whose only output is a mask, and channel extract declared
  an index of up to three on a three-channel image. Both enumerations and ranges are now
  correct, both refusals moved from run time to bind time, and the corresponding guards in
  the tools are gone. They were corrected in place rather than by a version bump because
  the catalog has never been published, so nothing binds these definitions anywhere.
- `nvision_reference_pipeline_tests` answers the exit-gate question directly. It runs the
  Pin Insert training and inspection flows of plan Section 12 as a chain of ordinary
  catalog tools on a generated fixture, learns a two by three layout, and measures a
  shifted fixture as deviating by exactly the root of five. A second test asserts every
  identity the chain used resolves from the built-in catalog, so adding a tool named after
  a product to make the flow work would be visible rather than quiet.
- Catalog coverage is 43 of 66 definitions. Image Processing, Location and Geometry, and
  Measurement are complete; Clustering and Structure is complete apart from point grid
  inspect.

## Current Status

M3 is complete and merged into `develop` by `d1faa1d`. The generic OpenCV vision foundation
implements 43 of the 66 catalog definitions, both reference grid flows run end to end on
those tools alone, and the full gate is green on `develop` after a clean rebuild.

`develop` is ahead of `origin/develop` by the whole milestone and has not been pushed.

## Next Actions

1. Start M4, the generic model and recognition foundation: `.nvmodel` integrity and
   manifest validation in C++, a model registry with instance-owned sessions, the native
   ONNX Runtime CPU backend, detector and recognizer pre- and post-processing, CTC
   decoding, configurable orientation search, and the nine Recognition and Text tools.
2. Resolve the external model provenance and license question first. M4 is the milestone
   that would otherwise want to import the Wire Marker models, and no license evidence has
   been found beside them.
3. Push `develop` when you are ready; this session did not push.
4. Run the Linux native headless build, now carried as an M11 release gate. WSL Ubuntu here
   has g++ 13.3 but neither `cmake` nor OpenCV, and `sudo` needs a password, so the
   packages could not be installed non-interactively:
   `wsl -d Ubuntu -- sudo apt-get install -y cmake libopencv-dev`, then
   `wsl -d Ubuntu -- bash -lc "cd /mnt/d/src/ai-workspace/06_Repositories/NTech && cmake -S . -B artifacts/native/linux-host -DCMAKE_BUILD_TYPE=Release -DNVISION_BUILD_TESTS=ON && cmake --build artifacts/native/linux-host && ctest --test-dir artifacts/native/linux-host"`.
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
- `PortValue` covers the port data kinds that have a native representation today.
  `point_grid_model`, `point_grid_result`, `overlay`, `evidence`, and `any_metadata` have
  no factory yet and gain one with the slice that needs them.
- Feature matching is tested for the contracts it must keep rather than for a number of
  matches, because how many correspondences a detector finds on a synthetic fixture is not
  a property worth pinning. The homography test builds its correspondences by hand so its
  expectation stays exact.
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

- Full `scripts/check.ps1 -Configuration Release`: passed after every slice, and again on
  `develop` after the merge from a deleted build directory. That run reported 0 native
  warnings under `/W4 /WX`, 13 CTests passed and 0 failed, 119 .NET tests passed and 0
  failed, and a clean `dotnet format --verify-no-changes`, against OpenCV 4.14.0.
- Portability audit in place of the Linux compile: no native source outside the C ABI
  export macro uses a Windows-only header or construct, that macro already has a GCC
  branch, no source uses a backslash path separator, and every quoted include resolves
  case-sensitively. This is an audit and not a compile, so the gate item stays open.
- Reference pipeline check: swapping the colour range bounds left nothing to find and
  failed the pipeline at its first assertion, so the chain genuinely executes rather than
  asserting over empty results.
- OpenCV check: with `OpenCV_DIR=D:\setups\opencv\build`, configure reported
  `Found OpenCV 4.14.0`, `nvision_vision` linked it, `opencv_world4140.dll` was copied
  into the native test output directory, and the gate stayed green.
- This is the first session in which the managed half of the gate actually ran.
- Negative check: removing the crop transform output made `nvision_tool_execution_tests`
  fail with `OUTPUT_CONTRACT_VIOLATION`, and restoring it returned the suite to green.
- Boundary check after the relocation: no header under `native/core`, `native/acquisition`,
  or `native/abi` includes anything from `nvision/workflow`, and no include of the old
  `nvision/core/` paths for the relocated files remains anywhere in `native/`.
