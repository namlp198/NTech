# Active Task

## Current Task

M3 — Generic OpenCV Vision Foundation — Complete and merged into `develop`

## Goal

Implement the 44 catalog definitions M3 owns behind their existing contracts, refining
parameter sets against real behaviour, with no public contract exposing an OpenCV type.

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

1. Start M4, the generic model and recognition foundation: `.nvmodel` integrity and
   manifest validation in C++, a model registry with instance-owned sessions, the native
   ONNX Runtime CPU backend, detector and recognizer pre- and post-processing, CTC
   decoding, configurable orientation search, and the nine Recognition and Text tools.
2. Resolve the external model provenance and license question before M4 imports anything.
   It is the milestone that would otherwise want those assets.
3. Run the Linux native headless build once WSL has `cmake` and `libopencv-dev`. It is
   carried as an M11 release gate rather than an open M3 item.

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

`scripts/check.ps1 -Configuration Release` with `OpenCV_DIR` set is green: 13 native
CTests, 119 .NET tests, and a clean `dotnet format --verify-no-changes`.

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
- Catalog definitions are corrected in place while the catalog is unpublished. Once it
  ships, a contract change requires a version bump instead.
- External Wire Marker model and Pin Insert asset provenance/license evidence is still
  unresolved. Do not import or redistribute those assets until it is.
- Pin Insert golden/regression images are still unavailable, so it remains an
  architectural contract oracle rather than a parity suite.
- Repository line endings: committed blobs still contain CRLF while `.gitattributes`
  declares `* text=auto eol=lf`. A full Release build leaves the working tree clean, so
  this is latent. `git add --renormalize .` as its own commit remains the clean fix.
