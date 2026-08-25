# Active Task

## Current Task

N1 — Native Frame, File Camera, and Acquisition Lifecycle — Complete

## Goal

Move authoritative Frame/PixelFormat contracts, buffer ownership, deterministic File
Camera replay, and bounded acquisition lifecycle from the managed compatibility path
into the portable C++20 Engine.

## Completed in Current N1 Slice

- Added immutable native Frame, PixelFormat, status, timestamp quality, metadata, and
  read-only FrameBuffer types with dimension/stride/length validation.
- Added shared native FrameLease ownership; the buffer is released only after the final
  internal or C ABI frame owner releases it.
- Added deterministic ASCII PGM P2 File Camera replay with ordered/looping sources,
  reconnect reset, stable frame IDs, exact metadata, and normalized Mono8 pixels.
- Added connect/acquire/disconnect state, missing-source faulting, invalid-state and
  end-of-stream failures, bounded lock/I/O processing, and cancellation checks.
- Extended C ABI v1 with opaque camera, frame, and cancellation handles; fixed-layout
  request/info structures; zero-copy buffer view; bounded UTF-8 metadata; and explicit
  status/error mapping.
- Extended the .NET 10 facade with `NativeFileCamera`, `NativeFrameLease`, `SafeHandle`
  ownership, cancellation bridging, explicit diagnostic pixel copy, and typed metadata.
- Added direct native, C ABI, architecture, and managed interop coverage while retaining
  the managed acquisition implementation as the P2 behavior oracle.

## Next Steps

1. N2: port `.nvmodel` package/integrity validation and model registry/session ownership
   to C++.
2. Port classification preprocessing and postprocessing to native golden-tested code.
3. Integrate ONNX Runtime through its native C/C++ API and remove managed production
   inference after parity and performance gates pass.
4. N3: port Workflow compilation/execution and the Wire Marker inspection path.

## Acceptance Criteria

- [x] Complete native frames reject invalid shape/stride/buffer combinations.
- [x] Native buffer ownership survives retained leases and ends at the final release.
- [x] File Camera replay, loop, reconnect, metadata, and pixels match the managed oracle.
- [x] Invalid state, missing source, timeout, cancellation, and end-of-stream are explicit.
- [x] C ABI camera/frame/cancellation ownership is opaque and version-compatible.
- [x] P/Invoke exercises the real native implementation through `SafeHandle`.
- [x] Native and managed acquisition/parity tests pass.
