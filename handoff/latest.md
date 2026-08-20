# Latest Handoff

## Date

2026-08-20

## Branch

`codex/native-engine-foundation`

## Completed

- Audited the complete documentation set and all production source projects against
  the approved native-first product boundary.
- Accepted ADR-0005: portable C++20 Core/Engine, C ABI + P/Invoke for embedded use,
  and a supervised gRPC Engine Host for production Runtime deployments.
- Rebaselined the affected product, architecture, Workflow, plugin, camera, AI,
  Runtime, Studio, roadmap, implementation, and repository-structure documents.
- Added the CMake native tree with `nvision_core`, the `nvision_engine` shared C ABI
  library, and native lifecycle/ABI/capability tests.
- Added C ABI v1 with opaque handles, explicit retain/release, bounded UTF-8 buffers,
  status codes, ABI negotiation, and thread-local last-error diagnostics.
- Added the .NET 10 `NTech.NVision.Native` SDK using source-generated P/Invoke and
  `SafeHandle`, plus three real native interop tests.
- Added the initial Engine Host protobuf contract for capability, package, inspect,
  cancel, state, and shared-frame operations.
- Integrated native configure/build/test into the standard repository scripts.
- Added repository LF policy consistent with `.editorconfig`.

## Current Status

N0 is complete. The native foundation is executable and the managed SDK loads the
real C ABI library. The existing managed P0-P2 implementation is intentionally still
present as the behavior oracle; it is not the final production Engine.

## Next Actions

1. Port Frame/PixelFormat/native buffer ownership and File Camera acquisition to C++.
2. Add ABI operations and managed parity tests for that acquisition slice.
3. Port preprocessing and ONNX Runtime execution to C++.
4. Implement the gRPC Engine Host only after the embedded ABI reaches Wire Marker
   parity, so both boundaries delegate to the same native application services.

## Warnings

- The protobuf is a reviewed transport contract scaffold; generated stubs and the
  production host process are intentionally deferred until native functional parity.
- The managed Workflow, acquisition, and ONNX implementations must not gain new
  canonical Engine behavior during migration.
- Large image payloads must remain in native/shared memory; do not serialize image
  bytes through ordinary gRPC messages.
- The local .NET 10.0.200 SDK under ignored `artifacts/dotnet` was used because the
  machine-wide SDK was .NET 9.

## Verification

- Native Release build: passed with warnings-as-errors.
- CTest: 1 passed, 0 failed.
- .NET Release build: 0 warnings, 0 errors.
- .NET tests: 117 passed, 0 failed, 0 skipped.
- Native P/Invoke integration: ABI negotiation, capability UTF-8, and handle disposal
  all passed against `nvision_engine.dll`.
