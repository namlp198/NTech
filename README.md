# NTech NVision

NVision is NTech's industrial Machine Vision platform for building, deploying, operating, and managing automated visual inspection systems.

The repository is a capability-oriented monorepo. Studio, Runtime, SDKs, plugins, schemas, tests, and reference applications share the same platform contracts while remaining independently evolvable.

## Current Status

P0 — Repository and Foundation, P1 — Core Domain and Workflow Engine, and **P2 — Wire Marker Headless Vertical Slice** are complete.

ADR-0005 rebaselines the production architecture as native-first. The existing managed
P0–P2 path remains a compatibility oracle while canonical Core/Engine behavior migrates
to C++20. C#/.NET 10 remains the product plane for Studio, Runtime supervision/UI,
authoring, persistence, and business software.

N1 is complete: native C++ now owns immutable Frame/PixelFormat contracts, read-only
buffer lifetime, retain/release frame leases, deterministic P2 File Camera replay,
camera connect/acquire/disconnect state, timeout/cancellation, and acquisition errors.
The .NET SDK exercises these capabilities only through the versioned C ABI.

M1 foundational contracts are complete and merged into `develop`.
Native Core now has typed stable identifiers, semantic versions, generic port data kinds,
coordinate-space-aware geometry and transforms, and separate technical execution status
versus inspection verdict.

Native acquisition also provides zero-copy `ImageView` contracts. A view retains its
source Frame lease, respects stride and ROI bounds, and carries frame-specific source/
local coordinate spaces with explicit transforms.

Resource, model, template, calibration, and dictionary bindings now share one immutable
native reference contract keyed by stable ID, exact semantic version, resource kind,
canonical SHA-256, and expected content length.

Immutable execution and inspection-result aggregates now enforce that technical failures
remain Indeterminate, contain matching execution errors, and never carry an OK/NG quality
decision. Evidence is content-addressed, uniquely identified, and spatial evidence must
declare its coordinate space.

M2 has started with a versioned native Tool Catalog contract. Tool definitions now bind
stable type identity and exact semantic version to typed ports, catalog metadata,
built-in/plugin origin, and execution characteristics. Exact registry resolution preserves
unresolved ID/version requests, while category paths remain display metadata rather than
persisted identity.

The Tool Catalog now also carries declarative parameter/property-editor schemas. Scalar,
enum, array, coordinate-aware geometry, ROI, and versioned resource/model/calibration
fields have type-safe defaults and editor metadata. Conditional fields are validated as
an acyclic dependency graph before a tool definition can enter the registry.

The first executable inspection path now runs deterministic File Camera frames through a compiled typed Workflow, real ONNX Runtime CPU inference, immutable result finalization, image evidence storage, and SQLite history. It proves the reference platform path and error semantics; the included synthetic model and PGM fixtures are engineering regression assets, not production inspection content.

## Read First

1. [Platform Foundation](docs/product/NVISION_PLATFORM_FOUNDATION.md)
2. [Documentation Index](docs/README.md)
3. [Implementation Plan](docs/implementation/13_IMPLEMENTATION_PLAN.md)
4. [Repository Structure](docs/architecture/14_REPOSITORY_STRUCTURE.md)
5. [Native Engine Architecture](docs/architecture/15_NATIVE_ENGINE_ARCHITECTURE.md)

Contributors using AI-assisted workflows must also read [docs/README_AI_WORKFLOW.md](docs/README_AI_WORKFLOW.md).

## Repository Capabilities

```text
apps/       Executable products and composition roots
engine/     Shared acquisition, vision, AI, workflow, and inspection behavior
native/     Canonical C++20 Core/Engine, C ABI, Engine Host, and native tests
sdk/        External embedding and control surfaces
plugins/    Replaceable vendor/backend/protocol implementations
schemas/    Versioned durable contracts
tests/      Verification organized by purpose
samples/    Reference applications and regression fixtures
tools/      Engineering, migration, packaging, and diagnostics utilities
docs/       Product and engineering knowledge
```

## Developer Commands

Windows is the initial development and deployment platform.

```powershell
.\scripts\restore.ps1
.\scripts\build.ps1
.\scripts\test.ps1
.\scripts\check.ps1
```

`build.ps1` and `test.ps1` include the native CMake/CTest pipeline.

Shell wrappers are retained for compatible CI and future Linux Runtime work.

## Architecture Guardrails

- Canonical Core/Engine execution is portable C++20.
- Runtime Domain has no UI, database, OpenCV, AI-backend, or vendor-SDK dependency.
- Engine has no Studio or Runtime UI dependency.
- OpenCV/ONNX Runtime hot paths are native; C# clients use C ABI/P/Invoke or Engine Host gRPC.
- Vendor integrations live behind versioned plugin contracts.
- Studio test mode and Runtime use the same Engine.
- Editable `.nvision` source is published as immutable `.nvpkg` for production.
- Technical failures never become valid-looking `OK` or ordinary `NG` results.

## License

License and distribution policy will be defined before external release.
