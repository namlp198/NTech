# ADR-0005 — Native-First Core and Engine

**Status:** Accepted
**Date:** 2026-08-20

## Context

NVision must execute performance-critical industrial vision workloads on Windows x64,
Linux x64, and Linux ARM64/Edge targets. The initial P0–P2 implementation proved the
required behavior in C#/.NET 10, but it placed canonical Domain, Workflow, Acquisition,
AI, and Inspection execution inside managed projects.

That implementation remains a behavioral reference. It is not the long-term production
language boundary.

## Decision

NVision adopts a native-first execution architecture:

- canonical Core, runtime Domain semantics, Workflow Engine, Acquisition, image/frame
  ownership, performance-critical Inspection nodes, AI preprocessing/postprocessing,
  and ONNX Runtime/TensorRT execution are implemented in portable C++;
- C++20 is the minimum native language baseline;
- OpenCV algorithms are implemented through the native OpenCV C++ API, not OpenCvSharp;
- ONNX Runtime integrations use its native C/C++ API;
- Studio, Runtime UI, Runtime supervision, application services, authoring DTOs,
  persistence/query services, and other business-oriented software use C#/.NET 10;
- a small versioned C ABI with opaque handles is the stable in-process boundary;
- C# embedded and compatibility clients consume the C ABI through P/Invoke;
- production Runtime uses a supervised native Engine Host through gRPC;
- large frame payloads remain in native/shared memory and do not traverse ordinary
  protobuf serialization in the inspection hot path;
- native camera, AI, and performance-sensitive node plugins expose a versioned C ABI;
- C++/CLI is not a platform boundary because it is Windows-specific.

The same native engine library is used by the P/Invoke adapter and the Engine Host.
There is no separate Studio test engine.

## Consequences

- The managed P0–P2 engine remains temporarily as an executable compatibility oracle
  until native golden, ownership, error, and traceability tests reach parity.
- New Core/Engine behavior must not be added only to the managed implementation.
- Public ABI structures use fixed-width values, UTF-8, explicit ownership, explicit
  buffer sizes, status codes, and create/retain/release functions.
- C++ exceptions and STL types never cross the C ABI.
- P/Invoke calls are coarse-grained; per-pixel and per-node managed/native chatter is
  prohibited in the hot path.
- gRPC is the production control plane. Shared memory or native evidence files carry
  large images.
- Native builds and tests become mandatory for Windows x64, Linux x64, and Linux ARM64
  as those CI runners become available.

## Migration Rule

Managed engine projects may be removed only after the corresponding native component
passes behavior-parity, lifetime, fault, and performance gates.
