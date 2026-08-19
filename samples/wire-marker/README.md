# Wire Marker Reference Application

Wire Marker is the first NVision end-to-end reference application and a permanent regression asset.

The first required behavior is:

```text
TYPE_01 image + TYPE_01 Recipe -> OK
TYPE_04 image + TYPE_01 Recipe -> NG / WRONG_TYPE
technical failure              -> ERROR
```

P2 provides a deterministic headless reference slice:

- ASCII PGM fixtures for `TYPE_01` and `TYPE_04` (text-reviewable test assets).
- `TYPE_01` and `TYPE_04` Recipe configurations.
- a development `.nvmodel` package with SHA-256 integrity metadata;
- an actual ONNX graph executed by the replaceable ONNX Runtime CPU plugin;
- explicit vertical-half preprocessing and softmax/top-1 postprocessing;
- typed Crop, Classify, Compare, and Decision nodes executed through the P1 Workflow compiler/runtime;
- immutable inspection results, image evidence, and SQLite reconstruction;
- golden cases for `OK`, `NG / WRONG_TYPE`, and technical `ERROR`.

The reference ONNX graph is intentionally tiny and deterministic: preprocessing reduces the 8×8 Mono8 fixture to left/right intensity features and the ONNX model exposes those scores. It verifies the platform boundary and traceability flow; it is not a production-trained classifier.

Run the complete demo on Windows:

```powershell
.\scripts\demo-wire-marker.ps1
```

Or inspect one image directly:

```powershell
dotnet run --project apps/nvision-cli/NTech.NVision.Cli -- inspect `
  samples/wire-marker/fixtures/type01.pgm TYPE_01 outputs/temporary/manual-wire-marker
```

The development package is directory-backed and stores its 146-byte ONNX artifact as Base64 so the complete model remains reviewable in source control. Archive import/export and production image codecs remain later packaging concerns; runtime model semantics and checksum validation are already enforced.
