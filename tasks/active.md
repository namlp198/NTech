# Active Task

## Current Task

M3 — Generic OpenCV Vision Foundation — Slices S0 through S2 complete

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
| S3 | Measurement, 11 tools | Not started |
| S4 | Clustering and Structure, 6 tools | Not started |
| S5 | Overlay and evidence primitives, 1 tool | Not started |
| S6 | Exit gate: parameter refinement, Pin Insert representation, Linux build | Not started |

Catalog coverage is 26 of 66 definitions. The remaining 22 outside M3 belong to M4
(Recognition and Text, 9) and M5 (Acquisition 5, Logic and Results 8).

## Completed in S0 through S2

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

## Next Steps

1. S3, Measurement, 11 tools. The measurement contract it needs already exists.
2. S4, Clustering and Structure, 6 tools. It needs a point-grid model and result contract.
3. S5, overlay and evidence primitives.
4. S6, close the M3 exit gate.

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
- [ ] Measurement, Clustering, and evidence tools are implemented and tested.
- [ ] Pin Insert source algorithms are representable without a Pin Insert engine class.
- [ ] The Linux native headless build compiles the vision library.

## Verification Baseline

`scripts/check.ps1 -Configuration Release` with `OpenCV_DIR` set is green: 12 native
CTests, 119 .NET tests, and a clean `dotnet format --verify-no-changes`.

## Open Items

- The catalog declares threshold modes `truncate` and `to-zero` on a tool whose output
  port is a mask. Both keep source intensity, so they produce an image rather than a mask.
  The tool refuses them today. Resolve in S6 by splitting the definition or bumping its
  version.
- External Wire Marker model and Pin Insert asset provenance/license evidence is still
  unresolved. Do not import or redistribute those assets until it is.
- Pin Insert golden/regression images are still unavailable, so it remains an
  architectural contract oracle rather than a parity suite.
- Repository line endings: committed blobs still contain CRLF while `.gitattributes`
  declares `* text=auto eol=lf`. A full Release build leaves the working tree clean, so
  this is latent. `git add --renormalize .` as its own commit remains the clean fix.
