# 15 — NVision Native Engine Architecture

**Status:** Accepted
**Version:** 1.0
**Date:** 2026-08-20
**Decision:** `ADR-0005`

## 1. Purpose

This document defines the approved language, process, ABI, memory, and deployment
boundaries between the portable NVision Engine and .NET product applications.

## 2. Language Ownership

### C++ execution plane

The following are canonical native components:

- Core status, errors, identifiers, versioning, and validation primitives;
- runtime Domain and compiled Project/Recipe/Workflow semantics;
- Workflow validation, compilation, scheduling, execution, cancellation, and trace;
- Acquisition, camera lifecycle, frame pools, leases, and pixel buffers;
- OpenCV image-processing algorithms and performance-sensitive nodes;
- AI model sessions, preprocessing, inference, and postprocessing;
- ONNX Runtime, TensorRT, OpenVINO, CUDA, and vendor SDK adapters;
- inspection execution and decision aggregation;
- native camera, AI backend, and performance-sensitive node plugins.

### C#/.NET 10 product plane

The following remain managed unless a measured requirement changes the decision:

- NVision Studio WPF UI, MVVM, docking, editors, and authoring workflows;
- NVision Runtime operator UI;
- Runtime Supervisor, lifecycle policy, business orchestration, and control API facade;
- editable ProjectFormat DTOs and Studio document state;
- SQLite result metadata, query services, retention policy, audit, and reporting;
- deployment management, Hub/Connect clients, licensing, and enterprise integration;
- managed SDK facades and generated protobuf contracts.

Managed authoring models are not the runtime source of truth. Native validation and
compilation are authoritative before test, publish, or activation.

## 3. Component Topology

```text
NVision Studio (.NET 10/WPF)
    ├── Project authoring DTOs and ViewModels
    └── Engine client ───────────────┐
                                     │ gRPC or P/Invoke
NVision Runtime Supervisor (.NET 10) │
    ├── readiness/alarm policy       │
    ├── persistence/query            │
    └── Engine Host supervision ─────┤
                                     ▼
NVision Engine Host (C++20) / nvision_engine shared library
    ├── Core + runtime Domain
    ├── Workflow Engine
    ├── Acquisition + native frame pool
    ├── Vision/OpenCV
    ├── AI/ONNX Runtime/TensorRT
    ├── Inspection execution
    └── native plugins
```

## 4. Shared Native Library

The portable engine is built as:

```text
Windows: nvision_engine.dll
Linux:   libnvision_engine.so
```

Internal modules use modern C++ APIs. External consumers do not link against exported
C++ class layouts or STL types.

## 5. Stable C ABI

The C ABI is the only supported in-process binary boundary.

Rules:

- `extern "C"` exported functions;
- ABI version negotiation before creating an engine;
- opaque handles for engine, package, plan, execution, frame, and result lifetime;
- explicit create/retain/release ownership;
- fixed-width integers and explicit struct sizes;
- UTF-8 strings represented by pointer plus length;
- caller-provided output buffers or explicit native release functions;
- stable numeric status/error codes with diagnostic UTF-8 detail;
- no exceptions, RTTI objects, STL containers, or compiler-owned allocators cross ABI;
- no callbacks from arbitrary native worker threads into UI code.

P/Invoke wrappers use `SafeHandle` and source-generated marshalling where practical.

## 6. P/Invoke Mode

P/Invoke is intended for:

- embedded engineering tools;
- compatibility and integration tests;
- CLI and offline validation;
- low-latency same-process deployment explicitly accepting native crash scope.

Calls are coarse-grained: create session, load package, compile, inspect, cancel, query
result. The managed layer never invokes native code per pixel or per inner algorithm step.

## 7. Production Engine Host

Production Runtime uses a native Engine Host supervised by the .NET Runtime service.

gRPC carries:

- capabilities and version negotiation;
- package prepare/validate/activate;
- recipe selection and warm-up;
- inspect/cancel/control requests;
- readiness, health, alarms, diagnostics, trace, and result metadata.

The Engine Host can restart without terminating the operator UI or managed supervisor.
The supervisor maps host failure to an explicit Runtime fault and performs controlled
known-good recovery.

## 8. Image Transport

Full-resolution image data does not use ordinary protobuf byte arrays in the hot path.

Preferred transport:

```text
camera/native decoder
    → native frame pool
    → opaque frame handle/shared-memory descriptor
    → native Workflow nodes
    → native evidence encoder or shared evidence reference
```

gRPC may carry compressed preview frames or thumbnails at a bounded rate. Shared-memory
descriptors include region identity, offset, length, format, dimensions, stride,
generation, and lease token. Stale generations are rejected.

## 9. Native Plugin ABI

Camera, AI backend, and performance-sensitive node plugins expose a C entry point that
returns a versioned function table. Plugin-owned resources are opaque. The host owns
loading policy, capability negotiation, timeouts, diagnostics, and final release.

Managed plugins remain suitable for non-hot-path exporters, reporting, MES business
integration, and authoring extensions. They cannot receive raw native frame ownership.

## 10. Runtime Result Boundary

The native engine returns immutable execution/result data. Protobuf is the canonical
process contract; JSON remains the durable human-readable result representation where
required. The managed persistence layer stores indexed fields and payloads without
reinterpreting inspection quality or technical error semantics.

## 11. Build and Portability

The native build uses CMake and C++20. Supported target intent is:

```text
Windows x64
Linux x64
Linux ARM64
```

Platform and vendor dependencies live behind adapters. Core, Domain, Workflow, and
generic frame contracts contain no Windows, WPF, vendor SDK, OpenCV, ONNX Runtime, or
database types.

## 12. Verification Gates

A native component replaces its managed reference only after:

1. golden behavior parity;
2. deterministic error/status parity;
3. ownership and leak tests;
4. cancellation/timeout tests;
5. traceability/version parity;
6. ABI compatibility tests;
7. measured p95/p99 performance and allocation budgets;
8. Windows and Linux build coverage for portable modules.

## 13. Migration Order

```text
Native foundation + C ABI
    → frame/buffer/acquisition/File Camera
    → model package + ONNX Runtime
    → Workflow Domain/validation/compiler/runtime
    → inspection nodes and Wire Marker execution
    → managed P/Invoke facade and parity tests
    → native Engine Host gRPC
    → production Runtime Supervisor integration
    → managed engine retirement
```

P3 Studio work may build managed authoring UI, but test, publish validation, and runtime
execution must target the native engine boundary.
