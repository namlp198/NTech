# Latest Handoff

## Date

2026-08-19

## Branch

`codex/nvision-foundation`

## Completed

- Approved and stored the implementation plan.
- Moved foundation documents into the canonical documentation taxonomy.
- Bootstrapped repository and workspace project context.
- Added accepted repository ADRs.
- Created a .NET 10 solution with capability-oriented project boundaries.
- Added central package/build/analyzer policy, scripts, NuGet config, and Windows CI.
- Added executable architecture rules.
- Completed P0 exit gate.
- Started P1 with result/error/validation primitives and initial strong IDs.
- Implemented Project and Job with aggregate-level uniqueness and timeout invariants.
- Implemented WorkflowDefinition, typed port descriptors, nodes, and connections.
- Enforced graph construction invariants including exact port type compatibility, multiplicity, self-loop, and cycle rejection.
- Added semantic versions and typed scalar parameter values without raw `object` configuration paths.
- Added Recipe aggregation, Project-level Recipe uniqueness, and default/project/Recipe parameter precedence.
- Added exact-version node type metadata registry with built-in/plugin origin and execution characteristics.
- Added the seven-stage Workflow validation pipeline and a dedicated Engine.Workflow unit-test project.
- Added immutable ExecutionPlan compilation with deterministic topological ordering, bound Recipe parameters, dependencies, routing, and resource requirements.
- Added typed node execution contracts, execution-scoped port values, and the sequential Workflow runtime.
- Added deterministic fake Constant/Add/GreaterThan nodes and upgraded the CLI to run the complete validate-resolve-compile-execute flow.
- Added immutable per-node `AbortWorkflow` and `Continue` policies to the compiled plan.
- Added dependency-aware failure propagation, contextual error categories, and explicit skipped-node reasons.
- Added Workflow/node timeout hierarchy and cancellation hardening for both cooperative and non-cooperative executors.
- Added monotonic node/Workflow timing and structured trace events through an injectable trace sink.
- Isolated trace persistence failure as warning `TRC-001` instead of failing the Workflow.
- Generalized the runtime around a stable ready queue and remaining-dependency counters.
- Added `BoundedParallelWorkflowRuntime` with explicit `MaxConcurrency`; retained `SequentialWorkflowRuntime` as the same scheduler configured with one worker.
- Added synchronized port-value storage and serialized trace-sink delivery for parallel nodes.
- Preserved abort/continue, skip propagation, Workflow/node timeout, cancellation hardening, timing, and deterministic result ordering under concurrency.
- Upgraded the CLI demo to execute its independent constant nodes with bounded concurrency `2`.
- Added immutable Frame/PixelFormat/IFrameBuffer contracts and reference-counted FrameLease ownership.
- Added camera plugin contracts, deterministic File Camera PGM replay, and AcquisitionService hardening for timeout, cancellation, non-cooperative devices, invalid ownership, and late lease release.
- Added a development `.nvmodel` manifest/package loader with SHA-256 validation, exact immutable model registry, and model-slot resolution.
- Added a replaceable ONNX Runtime 1.28 CPU plugin and ran an actual ONNX graph through explicit preprocessing and softmax/top-1 postprocessing.
- Added Crop, Classify, Compare, and Decision nodes composed through the P1 Workflow registry, Recipe resolution, compiler, and sequential runtime.
- Added immutable canonical InspectionResult records and validation that prevents technical failures from becoming OK/NG.
- Added file image evidence with checksums and SQLite result persistence/reconstruction.
- Added TYPE_01/TYPE_04 fixtures and configurations, golden/technical/repeat integration tests, `inspect` CLI command, and repeatable demo script.

## Current Status

P0, P1, and P2 are complete. The first full inspection path runs File Camera → AcquisitionService → compiled typed Workflow → ONNX Runtime CPU → immutable InspectionResult → file/SQLite evidence. The repository builds with zero warnings/errors and all 113 tests pass.

## Next Actions

1. Review the P3 ProjectFormat and Studio MVP scope.
2. Define editable Wire Marker `.nvision` DTOs and migrations without coupling runtime domain to JSON.
3. Add immutable `.nvpkg` publication and reopen/validate round-trip before Studio editing.

## Warnings

- The reference ONNX model is deliberately synthetic and deterministic; it validates the platform flow, not production classification quality.
- P2 uses a directory-backed development `.nvmodel` with a Base64 artifact and text-reviewable PGM fixtures. Archive packaging and production image codecs are deferred.
- Recipe inheritance is represented by `BaseRecipeId` but intentionally not resolved in this MVP slice.
- Reference validation is an explicit pipeline stage but remains empty until Device/Model/Calibration reference value types exist.
- MVP parallel scheduling uses one overall concurrency limit. Resource-specific CPU/I/O/GPU queues and keyed device locks remain later optimizations.
- MVP policies are `AbortWorkflow` and `Continue`. `ReturnNG` remains an Inspection/product decision, and fallback remains explicit graph structure rather than a hidden generic runtime policy.
- A non-cooperative in-process executor can be detached after timeout/cancellation but cannot be forcibly terminated; blocking vendor SDKs may still require a vendor abort API or process isolation.
- NuGet restore requires access to the user NuGet configuration/cache in the sandboxed Codex environment.
- Real camera and PLC selections remain intentionally unresolved until P5.

## Verification

- `scripts/check.ps1`: passed.
- Release build: 0 warnings, 0 errors.
- Tests: 113 passed, 0 failed, 0 skipped.
- P1 CLI demo: `(2 + 3) > 4` produces `Decision: True` through the compiled bounded-parallel runtime (`MaxConcurrency = 2`).
- P2 CLI demo: `type01.pgm + TYPE_01 -> OK`; `type04.pgm + TYPE_01 -> NG / WRONG_TYPE`; injected backend failure persists `ERROR`.
