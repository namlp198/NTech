# Latest Handoff

## Date

2026-08-25

## Branch

`codex/native-acquisition`

## Completed

- Implemented canonical C++20 Frame, PixelFormat, status, timestamp quality, metadata,
  and immutable native FrameBuffer contracts under `native/acquisition`.
- Enforced positive dimensions, packed-row stride, complete-buffer size, timestamp, and
  overflow invariants before a native Frame can exist.
- Implemented shared native FrameLease ownership and verified the buffer survives retained
  owners and is destroyed after the final owner releases it.
- Implemented deterministic ASCII PGM P2 File Camera replay with ordered/looping sources,
  stable reconnect IDs, Mono8 scaling, metadata, and sequence reset.
- Implemented connect/acquire/disconnect lifecycle, fault state, invalid-state,
  missing-source, invalid-data, I/O, end-of-stream, timeout, and cancellation behavior.
- Extended C ABI v1 with opaque camera/frame/cancellation handles, explicit
  retain/release, fixed-layout frame info, bounded UTF-8 fields, and zero-copy buffer view.
- Extended `NTech.NVision.Native` with typed camera/frame facades and `SafeHandle`
  ownership. `CopyPixels()` is explicit and limited to compatibility/diagnostics.
- Expanded capabilities to Engine 0.2.0 with `native-frame`, `file-camera`, and
  `native-acquisition` features.
- Added native behavior/ownership tests, C ABI tests, architecture guards, and .NET
  end-to-end interop/parity tests using the Wire Marker fixtures.
- Updated native architecture, camera architecture, repository context, task, SDK,
  native, and test documentation.

## Current Status

N1 is complete. Native acquisition is now authoritative. The managed acquisition/File
Camera path remains unchanged as a compatibility oracle for the full P2 Wire Marker
regression until Workflow and inspection migration are complete.

## Next Actions

1. Port `.nvmodel` package validation, integrity, and model/session ownership to C++.
2. Port preprocessing/postprocessing and add native golden parity fixtures.
3. Integrate ONNX Runtime through the native API.
4. Keep full-resolution frames native; do not add managed hot-path buffer ownership.

## Warnings

- File Camera cancellation is cooperative at lock, pre-I/O, and post-parse boundaries;
  standard blocking filesystem reads cannot be forcibly interrupted portably.
- C ABI buffer pointers are valid only while a retained `nv_frame` handle exists.
- Frame IDs are stable within the native implementation but are not promised to equal
  the temporary managed oracle's SHA-256-derived GUID bytes.
- The production Engine Host remains a protobuf contract scaffold until native Wire
  Marker parity is complete.

## Verification

- Full `scripts/check.ps1`: passed.
- Native Release build with warnings-as-errors: passed.
- CTest: 2 passed, 0 failed.
- .NET Release SDK build: 0 warnings, 0 errors.
- .NET tests: 119 passed, 0 failed, 0 skipped.
- Native interop tests: Engine/camera lifetime, frame retain/release, metadata, pixels,
  looping, reconnect identity, cancellation, invalid state, and end-of-stream passed.
