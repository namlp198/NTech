# Latest Handoff

## Date

2026-08-28

## Branch

`develop`. Both feature branches are merged and neither is deleted:

- `codex/group-a-contract-corrections`, five commits, merged by `566e4e2`.
- `codex/native-onnxruntime-dependency`, branched from that one so M4 work could build on
  the collection contract, merged by `e4dd1cb`.

`develop` is nine commits ahead of `origin/develop` and has not been pushed. The previous
handoff said it was 28 commits ahead before this session, which was already untrue then.

## Completed

Corrected the four review findings that change public contracts, before M4 and M5 build on
them. Group A is the set whose cost grows the longer it is deferred. Group B, below, is
implementation-local and blocks nothing.

### `636b729` — an executable collection contract for plural ports

`allows_multiple` was a graph-level claim with no runtime equivalent. `PortValueMap` holds
one value per key, so a validated fan-in would have kept only the last value it was handed,
and several catalog ports named a plural output while declaring a singular kind.

Arity now belongs to the port declaration. A port declaring `allows_multiple` always
carries a `PortValue` collection, empty or not; a port that does not never carries one. An
implementation therefore never tests which it received. Collections do not nest, every
element carries the declared element kind, and an empty collection still declares what it
is empty of.

`ToolExecutionContext` rejects a mismatched input arity, `execute_tool` rejects a
mismatched output arity, and `bind_workflow_graph` reports `PORT_ARITY_MISMATCH` when a
collection output reaches a single-value input. The other direction needed no rule, and the
first draft that added one was wrong: a collection input gathers its sources in connection
order, contributing one element per single source and its elements in order per collection
source, so any mixture already has one defined order.

Capture Sequence `images`, Image Buffer `images`, Text Recognition `crops`, Text Split
`parts` and Barcode Reader `barcodes` became collections of their element kind, and Text
Consensus `candidates` became a collection of `text_result`.
`PortDataType::text_result_list` was removed: it was the only list kind with no core
aggregate behind it, and declaring it on a port that also allowed multiple sources meant
plural twice over. The other list kinds stay, because `PointList`, `ContourList`,
`DetectionList` and `MeasurementList` are single domain aggregates with invariants a
generic collection would not enforce, such as one shared coordinate space.

### `ab61844` — evidence that can be read without out-of-band knowledge

Evidence stored packed pixels and recorded only a kind, a digest, a length and a coordinate
space. Nothing said how wide the image was, how many channels it had, what a component held
or how the bytes were encoded, so no consumer outside the run that wrote it could
reconstruct the image.

`EvidenceMedia` describes the stored bytes rather than the raster they came from, because
the raster is gone by the time anyone reads the evidence. The media type is derived from
the encoding rather than stored, so the two cannot disagree. The channel vocabulary is
deliberately not the acquisition one: it says what a reader will find in the bytes, so an
undecoded Bayer pattern or an HSV raster is described truthfully instead of being passed
off as a picture a viewer would misread.

`EvidenceArtifact` enforces the rest. Image kinds must carry media and non-image kinds must
not, so a descriptor always means a raster. Packed evidence whose length disagrees with its
descriptor is refused at construction, which is a real invariant: 16 by 8 mono8 pixels are
128 bytes and nothing else. `EvidenceSink::store` returns `StoredEvidence`, the integrity
and the storage reference together, because the storage key is only what the tool asked for
and the store is the only thing that knows where the bytes really went.

### `32d36a0` — promoted parameters resolved by full schema assignability

Promotion compared `ParameterKind` and asked whether the internal schema accepted the
exposed default. That admits an exposed enum of `{a, b}` into an internal enum of `{a}`:
the bind is clean, the default is fine, and the first Recipe that chooses `b` fails at
execution instead of at authoring time.

`ParameterType::is_assignable_to` compares whole accepted domains, and is deliberately
conservative. An unbounded source never fits a bounded target, an unbounded array count
never fits a bounded one, and a target step must be matched exactly, because proving that a
coarser grid lands on a finer one needs the origin as well as the spacing. Refusing a
domain it cannot prove contained is the safe direction; admitting one it cannot is how the
original defect happened.

### `7e4b20d` — execution characteristics that promise only what tools do

Every catalog definition claimed cancellation and timeout support, because those were the
constructor defaults and the catalog passes no characteristics. Neither claim was true. No
implementation observes the token while it runs, and none bounds its own work to the
budget; the engine checks the token before invoking and measures elapsed time after
returning, which diagnoses an overrun rather than preventing one.

Both defaults are now false, so a definition that says nothing promises nothing. A catalog
test pins that no built-in claims either, and is meant to fail the day one starts to, so
that the claim and the cooperative checkpoints justifying it arrive together.

### `ddd887e` — ONNX Runtime as a located native dependency

The native plane had no ONNX Runtime at all. The only one in the repository was the managed
NuGet reference, which copies runtime binaries into `bin/` and leaves the C++ headers in
the package, so nothing the C++ build could see. `nvision_public_contract_tests` already
forbade ONNX Runtime spellings in public headers, which is how you can tell the intent was
recorded long before the dependency existed.

Neither the official archives nor the NuGet package ship a CMake config package, so
`cmake/FindONNXRuntime.cmake` is required rather than a convenience. It resolves from
`ONNXRuntime_ROOT`, the `ONNXRUNTIME_ROOT` environment variable, or the repository-local
staging directory, in that order. No path in the repository names a machine, which is the
same rule OpenCV follows through `OpenCV_DIR`.

`scripts/install-onnxruntime.ps1` stages the CPU package the managed plane is already
pinned to, in the layout the official archives use, so either source resolves against one
find module. Both planes then run one ONNX Runtime version. That matters specifically
because the managed P2 path is still the behavior oracle for native recognition work: a
difference in results should mean a difference in our code, not in the runtime underneath
it. The script downloads nothing; if the package is not cached it names the command that
fetches it and stops.

`nvision_ai` is the seam M4's backend hangs off, and no larger than needed to prove the
dependency works. `InferenceSession` owns one model session; it loads from bytes rather
than a path, because a model arrives inside a versioned package whose integrity was already
checked and re-reading the file would be checking one thing and running another. Tensor
specs report a free dimension as -1 rather than guessing it. Running is deliberately narrow
— one float input to one float output — and refuses anything else instead of pretending to
be the general case.

ONNX Runtime stays PRIVATE, and `ai/include` was added to the public contract guard's
include roots so the new area is covered rather than assumed.

## Current Status

The full gate is green: 0 native warnings under `/W4 /WX`, 14 native CTests, 119 .NET
tests, and a clean `dotnet format --verify-no-changes`, against OpenCV 4.14.0, ONNX Runtime
1.28.0, and the repository-local .NET SDK 10.0.200.

## Surfaced rather than closed

Correcting the collection contract exposed a real gap. The Wire Marker graph of plan
Section 12.1 fed the recognizer's crop collection straight into single-image quality tools,
and the arity rule now refuses that connection, correctly. Measuring each crop needs
per-element fan-out over a collection, which is scheduling rather than a tool, so M5 owns
it. The reference graph measures print quality through an explicit Crop whose ROI comes
from the Recipe: expressible today, and where the plan puts product geometry anyway.

Do not close that gap with a per-kind element-selector tool. The catalog has no polymorphic
port, so it would take one tool per data kind, which is a worse contract than the gap.

## Next Actions

1. Push `develop` when you are ready. This session merged both branches but did not push,
   and the merged state is verified only on this machine.
2. Work Group B, the five remaining review findings. None blocks a milestone contract, but
   two of them sit directly on M4's coordinate path and should come first: the resize
   pixel-centre convention (2), because detector preprocessing resizes to the model input
   size and has to map boxes back to the source, and the missing affine transform output
   (3), because recognition crops are rectified through it. ECC (1), homography validation
   (4) and the immutability guard (5) are Pin Insert alignment concerns M4 never touches.
   1. ECC refinement passes and labels its transform in the direction opposite to the
      OpenCV contract. `findTransformECC(template, input, warp)` yields a matrix used with
      `WARP_INVERSE_MAP`, so it maps reference to image, while the tool requires and labels
      image to reference. Homography Estimate a few lines above it is correct, which is
      what makes this a local slip rather than a convention. Settle it with non-identity
      translation and rotation fixtures; the current test uses two identical rasters and an
      identity start, so it cannot detect an inversion.
   2. Resize publishes a pure scale. OpenCV's `INTER_LINEAR` uses pixel centres, so the
      exact mapping is `x_src = (x_dst + 0.5) * s - 0.5` and the published transform is
      short by `(s - 1) / 2`. Declare the convention and test a non-integer scale.
   3. Affine Transform produces a raster in a new coordinate space and publishes no
      transform, so downstream geometry cannot map back. Perspective Warp already does. It
      also fixes the output canvas to the input size, so content can be clipped silently.
   4. Homography Validate reads only the affine 2x2 block; `values[6]` and `values[7]` are
      never read, so a matrix with extreme projective distortion still validates.
   5. `interop::borrow` hands immutable bytes through a mutable `cv::Mat`. Nothing mutates
      an input today, so this is a guard rather than a fix.
3. Then start M4. Two of its prerequisites are now met: the collection model exists, and
   the ONNX Runtime backend links and runs. Its first slice is the one still missing —
   `text_result` and `barcode_result` have no native representation and `PortValue` has no
   factory for them. After that, `.nvmodel` integrity and manifest validation in C++, where
   the existing managed manifest at `samples/wire-marker/models/wire-marker-1.0.0.nvmodel`
   is the oracle to validate against.
4. Resolve the external model provenance and license question before M4 imports anything.
5. Run the Linux native headless build, carried as an M11 release gate.

## Warnings

- OpenCV moved on this machine during the session. It was at `D:\setups\opencv` and is now
  at `D:\opencv\build`, which is why a mid-session configure failed. Nothing in the
  repository names either path, so only the `OpenCV_DIR` you export has to change; the
  failure is worth knowing about because it looks like a build regression and is not one.
- The ONNX Runtime staged into `artifacts/onnxruntime` is the CPU package. Two GPU
  distributions exist on this machine under `D:\src\learn_opencv_all\Libraries`, left by
  the external Wire Marker project. Do not point `ONNXRUNTIME_ROOT` at them: the plan puts
  the production backend on CPU, and a GPU build would make results depend on which
  providers happened to load.
- `InferenceSession::run` is single-input, single-output, float only. That is enough for a
  classifier head and for the smoke test, and not enough for a text detector. Widen it when
  M4 has a real consumer to shape it against, rather than guessing the general case now.

- `PortValue` still has no representation for `text_result`, `barcode_result`,
  `point_grid_result`, `defect`, `defect_list`, `frame`, `image`, `bounding_box`, `polygon`
  or `any_metadata`. "43 of 66 implemented" counts definitions, not round-trippable
  contracts, and the two numbers should not be reported as the same claim.
- `EvidenceSink` is a changed contract. Any implementer outside this repository returns
  `StoredEvidence` now, not `ResourceIntegrity`.
- Evidence is still written as raw packed pixels. The contract now describes them
  truthfully, and encoding to PNG or TIFF is a change of one enum value and the bytes handed
  to the sink, but nothing encodes yet.
- The execution budget is still enforced after the fact. It measures and reports; it does
  not interrupt. That is now stated in the header rather than implied.
- Catalog definitions were corrected in place because the catalog is unpublished. Once it
  ships, a contract change requires a version bump instead.
- Pin Insert golden/regression images remain unavailable, so it is an architectural
  contract oracle rather than a parity suite.
- No license or provenance evidence has been found beside the external Wire Marker
  project/models or the provided Pin Insert project. Do not import or redistribute them.
- Repository line endings are still inconsistent in committed blobs while `.gitattributes`
  declares `* text=auto eol=lf`. A full Release build leaves the working tree clean, so it
  stays latent. `git add --renormalize .` as its own commit remains the clean fix.
- These are internal C++ APIs. Do not expose them through C ABI or gRPC until M8.

## Verification

- Full `scripts/check.ps1 -Configuration Release`: green after all five commits, and again
  on `develop` after both merges from a deleted build directory. That run reported 0 native
  warnings under `/W4 /WX`, 14 CTests passed and 0 failed, 119 .NET tests passed and 0
  failed, and a clean `dotnet format --verify-no-changes`.
- Negative check on the collection contract: removing the output arity check made
  `nvision_tool_execution_tests` fail, and restoring it returned the suite to green.
- Negative check on assignability: restoring the kind-only promotion comparison made
  `nvision_workflow_contract_tests` fail on both the widened ROI shape set and the
  unbounded range, and the assignability check returned it to green.
- Evidence check: the tool's own test now asserts the descriptor against what the sink was
  handed, so `packed_length_bytes()` and the stored length are compared rather than
  assumed, in both the plain and the annotated path.
- The arity rule found the crop connection in the Wire Marker reference graph before any
  test was written for it, which is the first evidence that the contract does real work.
- ONNX Runtime was configured from a deleted build directory, so the find module was proven
  rather than inherited from a CMake cache.
- Negative check on the runtime: removing `onnxruntime.dll` from the native test output
  makes `nvision_inference_tests` fail to start, so it loads the real runtime.
- Negative check on the guard: adding an `Ort::` spelling to the new public header makes
  `nvision_public_contract_tests` fail, so it really covers `ai/include`.
- The inference test asserts equality rather than plausibility. The fixture is a 146-byte
  identity model, so what comes out must be exactly what went in, and a wrong tensor layout
  or an output read at the wrong offset cannot pass.
