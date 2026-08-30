# Active Task

## Current Task

M4 preparation. Group A contract corrections are complete on
`codex/group-a-contract-corrections`; the ONNX Runtime native dependency is complete on
`codex/native-onnxruntime-dependency`, branched from it. Neither is merged into `develop`.

## Goal

Correct the four review findings that change public contracts, before M4 and M5 build on
them. Group A is the set whose cost grows the longer it is deferred; Group B is
implementation-local and can follow without blocking the milestones.

## Outcome

Four commits, each green on the full gate:

| Commit | Correction | Guide gate |
|---|---|---|
| `636b729` | Executable collection contract for plural ports | §6.1 |
| `ab61844` | Self-describing evidence media and storage reference | §6.4 |
| `32d36a0` | Full schema assignability for promoted parameters | §6.6 |
| `7e4b20d` | Truthful cancellation and timeout characteristics | §6.5 |

- Arity now belongs to the port declaration. A port declaring `allows_multiple` always
  carries a `PortValue` collection, empty or not, and a port that does not never carries
  one. Collections do not nest, and an empty one still declares its element kind.
- `bind_workflow_graph` reports `PORT_ARITY_MISMATCH` when a collection output reaches a
  single-value input. A collection input needs no rule: it gathers its sources in
  connection order, so any mixture of single and plural sources has one defined order.
- Capture Sequence `images`, Image Buffer `images`, Text Recognition `crops`, Text Split
  `parts` and Barcode Reader `barcodes` became collections. Text Consensus `candidates`
  became a collection of `text_result`, and `PortDataType::text_result_list` was removed:
  it was the only list kind with no core aggregate behind it, and declaring it on a port
  that also allowed multiple sources was the double-plural the collection model resolves.
- `EvidenceMedia` describes the stored bytes rather than the raster they came from, in a
  media vocabulary rather than the acquisition one. Image evidence must carry it and
  non-image evidence must not, and packed evidence whose length disagrees with its
  descriptor is rejected at construction.
- `EvidenceSink::store` returns `StoredEvidence`: the integrity that identifies the bytes
  and the reference that finds them again, both from the store, because the storage key is
  only what the tool asked for.
- `ParameterType::is_assignable_to` compares whole accepted domains. It is deliberately
  conservative: it refuses a domain it cannot prove contained rather than admitting one it
  cannot.
- `ToolExecutionCharacteristics` defaults to promising nothing. A catalog test pins that no
  built-in claims cancellation or timeout, so the claim and the cooperative checkpoints
  that justify it must arrive together.

## ONNX Runtime native dependency — `ddd887e`

The native plane had no ONNX Runtime at all. The only one in the repository was the managed
NuGet reference, which copies runtime binaries into `bin/` and leaves the C++ headers in
the package. `nvision_public_contract_tests` already forbade ONNX Runtime spellings in
public headers, so the intent had been recorded long before the dependency existed.

- `cmake/FindONNXRuntime.cmake` resolves a distribution from `ONNXRuntime_ROOT`, the
  `ONNXRUNTIME_ROOT` environment variable, or the repository-local staging directory. No
  path in the repository names a machine. A find module is required rather than a
  convenience: neither the official archives nor the NuGet package ship a CMake config.
- `scripts/install-onnxruntime.ps1` stages the CPU package the managed plane is already
  pinned to, in the layout the official archives use, so either source resolves. Both
  planes then run one ONNX Runtime version, which matters while the managed P2 path is the
  behavior oracle. The script downloads nothing.
- `nvision_ai` is the seam M4's backend hangs off, no larger than needed to prove the
  dependency works. `InferenceSession` owns one session, loads from bytes because a model
  arrives inside a package whose integrity was already checked, and reports tensors with
  free dimensions left as -1 rather than guessed. Running is one float input to one float
  output and refuses anything else.
- ONNX Runtime stays PRIVATE and `ai/include` is now scanned by the public contract guard.

Verified against ONNX Runtime 1.28.0, configured from a deleted build directory so the find
module was proven rather than inherited from a cache.

## Surfaced by the corrections

The Wire Marker reference graph fed the recognizer's crop collection straight into
single-image quality tools. That connection is now refused, correctly: measuring each crop
needs per-element fan-out over a collection, which is an M5 scheduling capability rather
than a tool. The graph measures print quality through an explicit Crop whose ROI comes from
the Recipe, which is expressible today and is where the plan puts product geometry anyway.
M5 owns the fan-out.

## Remaining review findings — Group B

Implementation-local, no contract change, safe to do in any order:

1. ECC refinement passes and labels its transform in the direction opposite to the OpenCV
   contract. `findTransformECC(template, input, warp)` yields a matrix used with
   `WARP_INVERSE_MAP`, so it maps reference to image, while the tool declares image to
   reference. Homography Estimate in the same file is correct, so this is a local slip.
   Needs non-identity translation and rotation fixtures to settle.
2. Resize publishes a pure scale. OpenCV's `INTER_LINEAR` uses pixel centres, so the exact
   mapping is `x_src = (x_dst + 0.5) * s - 0.5` and the published transform is short by
   `(s - 1) / 2`. Declare the convention and test a non-integer scale.
3. Affine Transform produces a new raster in a new coordinate space and publishes no
   transform, so downstream geometry cannot map back. Perspective Warp already does.
4. Homography Validate reads only the affine 2x2 block, so a matrix with extreme
   projective distortion still validates. It should evaluate mapped corners, the
   homogeneous denominator, orientation, convexity, area ratio and local scale.
5. `interop::borrow` hands immutable bytes through a mutable `cv::Mat`. Nothing mutates an
   input today; a source-byte immutability check after representative tools would keep it
   that way.

## M3 Historical Baseline

The sections below retain the completed M3 slice and verification record. They remain the
implementation baseline while the generalization guide controls entry into later work.

## Slice Plan

| Slice | Scope | Status |
|---|---|---|
| S0a | Computed image ownership, ADR-0007 | Complete, `5475413` |
| S0b | OpenCV required, interop substrate, public-contract guard | Complete, `fe432d4` |
| S1 | Image Processing, 15 tools | Complete, `43edcd8` |
| S2a | Location and geometry data contracts | Complete, `9f2c0d3` |
| S2b | Location and Geometry, 11 tools | Complete, `ab9b17c` |
| S3 | Measurement, 10 tools | Complete, `a4c2108` |
| S4 | Clustering and point grid, 6 tools | Complete, `4b72161` |
| S5 | Overlay and evidence primitives, 1 tool | Complete, `462b558` |
| S6 | Exit gate: parameter refinement, Pin Insert representation | Complete, `6df1505` |

Catalog coverage is 43 of 66 definitions. The remaining 23 outside M3 belong to M4
(Recognition and Text, 9) and M5 (Acquisition 5, Logic and Results 8, point grid inspect 1).

## Completed in S0 through S6

- `Image` is a computed raster that owns its pixels and carries no acquisition metadata,
  so a derived image can never be mistaken for a capture. `ImageSource` names which raster
  kind a view exposes. Recorded as ADR-0007.
- `ToolExecutionContext::output_image_id` derives an output image identity from the node
  and the port, so a repeated run reproduces the same coordinate space.
- OpenCV is a required dependency of `nvision_vision`, found through the standard CMake
  search honouring `OpenCV_DIR`. Verified against OpenCV 4.14.0.
- `opencv_interop` is the only place NVision image contracts meet OpenCV. It borrows a
  view as a `cv::Mat` without copying and copies results back into an owned `Image`. The
  pixel format is declared by the producing tool because OpenCV does not record meaning.
- `nvision_public_contract_tests` scans every public include root for OpenCV and ONNX
  Runtime spellings, so the no-third-party-type rule is enforced rather than trusted.
- All 15 Image Processing tools execute, each refusing what it cannot do correctly.
- `PixelFormat` gained `bayer_gr8`, `hsv8`, and `mono_float32`.
- Added the data contracts the geometry tools report through: `Contour`, `Measurement`,
  `Detection`, `Feature`, `FeatureSet`, `FeatureMatch`, and `MatchSet`. A contour may be
  degenerate where a polygon may not, a measurement in image units must name its
  coordinate space, and a match set carries both feature sets so a later node recovers the
  point pairs without the images.
- All 11 Location and Geometry tools execute. Coordinate space is checked wherever two
  inputs must agree, and algorithmic failure is separated from configuration failure.
- `PortValue::point_list` rejects a list that crosses coordinate spaces.
- 10 of the 11 Measurement tools execute. Lengths carry the coordinate space they were
  measured in; counts and quality figures name none. Comparing two rasters requires a
  shared format and extent.
- `PointGridModel` records where features are expected and nothing about what they mean.
  Tolerances belong to the recipe that inspects against it, not to the model.
- 5 of the 6 Clustering and Structure tools execute, together with grid deviation.
  K-Means fixes OpenCV's thread-local generator, DBSCAN is implemented directly and
  expands in input order, and both sorting paths are stable, so a repeated run produces
  identical results.
- `Overlay` is data rather than something already drawn, so no result depends on a drawing
  having happened. `EvidenceSink` lets a tool hand over bytes and receive the integrity
  that identifies them without learning where they are kept or computing the digest.
- Evidence Output executes. The artifact reports whether the raster was captured, computed
  or annotated, using the provenance ADR-0007 introduced.
- Corrected the two catalog definitions that promised what their contract cannot deliver.
  Threshold now offers only the modes that produce a mask, and channel extract bounds its
  index to the channels a three-channel image has. Both refusals moved from run time to
  bind time, which is where a schema violation belongs.
- `nvision_reference_pipeline_tests` runs the Pin Insert training and inspection flows of
  plan Section 12 as a chain of ordinary catalog tools on a generated fixture, and asserts
  every identity the chain used is a built-in definition.

## Next Steps

1. Merge `codex/group-a-contract-corrections`, then `codex/native-onnxruntime-dependency`,
   into `develop` when authorized. Neither is pushed.
2. Work Group B above. Two of the five sit on M4's coordinate path and should come first:
   the resize pixel-centre convention, because detector preprocessing resizes to the model
   input size and has to map boxes back, and the missing affine transform output, because
   recognition crops are rectified through it. ECC, homography validation and the
   immutability guard are Pin Insert concerns that M4 never touches.
3. Then M4 proper: `.nvmodel` integrity and manifest validation in C++, a model registry
   over instance-owned sessions, detector and recognizer pre- and post-processing, CTC
   decoding, configurable orientation search, and the nine Recognition and Text tools. The
   collection model M4 was waiting on exists, and the ONNX Runtime backend now links and
   runs. `text_result` and `barcode_result` still have no native representation and
   `PortValue` has no factory for them; that is M4's first slice.
4. Resolve the external model provenance and license question before M4 imports anything.
   The engineering fixtures in `samples/` are not affected: they are synthetic.
5. Run the Linux native headless build, carried as an M11 release gate. The staging script
   accepts `-Runtime linux-x64`, and the same package carries that runtime.

## Acceptance Criteria

- [x] A resolved tool definition can be bound to an implementation and executed.
- [x] An unimplemented definition is reported explicitly, never silently skipped.
- [x] Typed port values enforce the declared data kind at execution time.
- [x] A technical failure yields an execution error and never an OK/NG quality decision.
- [x] Cancellation and timeout are observable through the execution contract.
- [x] No public header or public contract exposes an OpenCV type, enforced by a test.
- [x] The native and managed halves of `scripts/check.ps1` are both green.
- [x] Image Processing tools are independently tested against exact properties.
- [x] Location and Geometry tools are implemented and tested.
- [x] Measurement tools are implemented and tested, except grid deviation.
- [x] Clustering and point-grid tools are implemented and tested, except point grid inspect.
- [x] Overlay and evidence tools are implemented and tested.
- [x] Pin Insert source algorithms are representable without a Pin Insert engine class.
- [ ] The Linux native headless build compiles the vision library. Carried to M11.

## Verification Baseline

`scripts/check.ps1 -Configuration Release` with `OpenCV_DIR` set is green: 0 native
warnings under `/W4 /WX`, 14 native CTests, 119 .NET tests, and a clean
`dotnet format --verify-no-changes`. The fourteenth test is `nvision_inference_tests`,
added with the ONNX Runtime dependency.

Negative checks performed rather than assumed: removing the output arity check makes
`nvision_tool_execution_tests` fail, and restoring the kind-only promotion comparison makes
`nvision_workflow_contract_tests` fail on both the widened ROI shape set and the unbounded
range. Both suites return to green once the checks are restored. Removing
`onnxruntime.dll` from the native test output makes `nvision_inference_tests` fail to
start, so it loads the real runtime rather than a stub, and adding an `Ort::` spelling to
the new public header makes `nvision_public_contract_tests` fail, so the guard really
covers `ai/include`.

## Open Items

- Point grid inspect is not implemented. It receives its model as a versioned resource
  rather than through a port, so it needs the resource binding that arrives with M5
  workflow execution. Its `point_grid_result` contract is unwritten for the same reason:
  it would have no producer to check against. Both are M5 items, and the coverage test
  asserts the gap rather than leaving it silent.

- The Linux native headless build has not been run, and is carried to M11 as a release
  gate rather than kept as an open M3 item. WSL Ubuntu on this machine has g++ 13.3
  but neither `cmake` nor OpenCV, and `sudo` requires a password, so the packages could not
  be installed non-interactively. What was checked instead: no native source outside the C
  ABI export macro uses a Windows-only header or construct, that macro already has a GCC
  branch, no source uses a backslash path separator, and every quoted include resolves
  case-sensitively. That is an audit, not a compile, and the gate item stays open until the
  build actually runs.

  ```bash
  wsl -d Ubuntu -- sudo apt-get install -y cmake libopencv-dev
  ```
- Per-element fan-out over a collection has no representation. A collection can be built,
  carried and gathered, but nothing applies a subgraph to each element, so a plural
  producer cannot feed a single-value consumer at all. That is deliberate: iteration is
  scheduling rather than a tool, and M5 owns it. Do not work around it by adding a
  per-kind element-selector tool; the catalog has no polymorphic port and one tool per
  data kind would be a worse contract than the gap.
- `PortValue` still has no representation for `text_result`, `barcode_result`,
  `point_grid_result`, `defect`, `defect_list`, `frame`, `image`, `bounding_box`,
  `polygon` or `any_metadata`. The catalog declares those kinds on ports that no
  implementation produces yet, so "43 of 66 implemented" counts definitions rather than
  round-trippable contracts. M4 and M5 own the kinds they first execute.
- Catalog definitions are corrected in place while the catalog is unpublished. Once it
  ships, a contract change requires a version bump instead.
- External Wire Marker model and Pin Insert asset provenance/license evidence is still
  unresolved. Do not import or redistribute those assets until it is.
- Pin Insert golden/regression images are still unavailable, so it remains an
  architectural contract oracle rather than a parity suite.
- Repository line endings: committed blobs still contain CRLF while `.gitattributes`
  declares `* text=auto eol=lf`. A full Release build leaves the working tree clean, so
  this is latent. `git add --renormalize .` as its own commit remains the clean fix.
