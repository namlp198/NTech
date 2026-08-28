# Latest Handoff

## Date

2026-08-28

## Branch

`codex/group-a-contract-corrections`, branched from `develop` at `68e5555`. Four commits,
not yet merged and not pushed. `develop` itself is level with `origin/develop`; the
previous handoff said it was 28 commits ahead and unpushed, which is no longer true.

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

## Current Status

The full gate is green: 0 native warnings under `/W4 /WX`, 13 native CTests, 119 .NET
tests, and a clean `dotnet format --verify-no-changes`, against OpenCV 4.14.0 and the
repository-local .NET SDK 10.0.200.

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

1. Merge `codex/group-a-contract-corrections` into `develop` when authorized. Nothing in it
   is pushed yet.
2. Work Group B, the four remaining review findings. None blocks a milestone contract.
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
3. Then start M4. The collection model it was waiting on exists; `text_result` and
   `barcode_result` still have no native representation and `PortValue` has no factory for
   them, so M4 owns those.
4. Resolve the external model provenance and license question before M4 imports anything.
5. Run the Linux native headless build, carried as an M11 release gate.

## Warnings

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

- Full `scripts/check.ps1 -Configuration Release`: green after all four commits.
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
