# Active Task

## Current Task

N0 — Native Engine Foundation and Managed Boundary — Complete

## Goal

Rebaseline NVision around one portable C++20 Core/Engine, expose it through a stable
C ABI for embedded P/Invoke clients, and define a gRPC Engine Host boundary for
production Runtime deployments.

## Completed in Current N0 Slice

- Accepted ADR-0005 and rebaselined the product, architecture, AI, device, Runtime,
  Studio, roadmap, implementation, and repository-structure documentation.
- Added a cross-platform CMake foundation and portable `nvision_core` C++ library.
- Added versioned C ABI v1 with opaque handles, retain/release ownership, bounded
  UTF-8 output, capability discovery, and thread-local diagnostics.
- Added the .NET 10 `NTech.NVision.Native` P/Invoke SDK using `LibraryImport` and
  `SafeHandle`, with real native integration tests.
- Added the initial production Engine Host protobuf contract, including lifecycle,
  inspection, cancellation, state, and shared-frame descriptors.
- Integrated native build/test into the repository build, test, and check scripts.
- Preserved the managed P0-P2 implementation as a behavior oracle during migration.

## Next Steps

1. N1: port Frame/PixelFormat/buffer ownership, File Camera, and acquisition lifecycle
   to C++ while retaining the existing managed tests as parity tests.
2. N2: port preprocessing and ONNX Runtime execution to C++, then remove production
   inference from the managed ONNX plugin.
3. N3: port Workflow compilation/execution and the Wire Marker inspection path.
4. N4: generate and implement the production gRPC Engine Host after C ABI parity.

## Acceptance Criteria

- [x] Native C++ builds with warnings-as-errors through CMake.
- [x] Native unit tests pass.
- [x] C ABI lifetime, ABI negotiation, capabilities, and diagnostics are tested.
- [x] .NET P/Invoke integration loads and exercises the real native library.
- [x] Production Engine Host protobuf contract is versioned in the repository.
- [x] Full native + managed repository check passes.
