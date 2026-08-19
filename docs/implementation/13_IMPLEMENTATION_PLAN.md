# 13 — NVision Implementation Plan

**Document Type:** Implementation Plan / Engineering Execution Baseline
**Product:** NTech NVision
**Status:** Approved — P0 Complete / P1 In Progress
**Version:** 1.0
**Date:** 2026-08-19
**Depends On:** `NVISION_PLATFORM_FOUNDATION.md`, documents `01` through `12`

---

# 1. Purpose

This document converts the NVision foundation and architecture set into an actionable engineering plan.

It defines:

- the canonical repository structure,
- the initial .NET solution and project boundaries,
- dependency rules,
- implementation phases and exit gates,
- the first executable vertical slice,
- initial contracts and class responsibilities,
- test and CI strategy,
- documentation migration,
- task ordering,
- risk controls,
- and the definition of done for the first production-capable release.

After this document is approved, implementation should be able to begin without another broad architecture round.

---

# 2. Current State

The repository currently contains only the NVision foundation documentation set:

```text
NVISION_PLATFORM_FOUNDATION.md
01_PLATFORM_ARCHITECTURE.md
02_DOMAIN_MODEL.md
03_WORKFLOW_ENGINE_ARCHITECTURE.md
04_PLUGIN_ARCHITECTURE.md
05_PROJECT_FILE_FORMAT.md
06_INSPECTION_RESULT_SCHEMA.md
07_CAMERA_ABSTRACTION.md
08_AI_ENGINE_ARCHITECTURE.md
09_RUNTIME_ARCHITECTURE.md
10_STUDIO_UI_ARCHITECTURE.md
11_DESIGN_SYSTEM.md
12_PRODUCT_ROADMAP.md
13_IMPLEMENTATION_PLAN.md
```

No solution, application, engine, plugin, schema, test, sample, or deployment package has been implemented yet.

---

# 3. Planning Rules

## 3.1 Source Document Precedence

When documents describe the same subject at different levels, use this precedence:

1. `NVISION_PLATFORM_FOUNDATION.md` defines product intent and canonical platform boundaries.
2. `01` through `11` define detailed subsystem contracts.
3. `12_PRODUCT_ROADMAP.md` defines dependency order and release gates.
4. This document defines the concrete implementation sequence.

If implementation discovers a true contradiction, record an ADR before changing a foundation rule.

## 3.2 Capability Structure Before Framework Structure

The repository root must communicate the NVision product ecosystem:

```text
apps
engine
sdk
plugins
schemas
tests
samples
tools
docs
```

Do not replace this with a generic root organized only as:

```text
src/core
src/infrastructure
src/ui
```

Clean Architecture and .NET project layering are applied inside the platform capabilities.

## 3.3 Vertical Slices Before Breadth

The first priority is one complete inspection path:

```text
File Camera
    -> Crop
    -> ONNX Classification
    -> Compare TYPE_01
    -> Decision
    -> Inspection Result
    -> SQLite and image storage
```

Do not build broad catalogs of unfinished nodes, devices, screens, or protocols.

## 3.4 Simulators Before Hardware

Implement and test:

- File Camera before an industrial camera,
- PLC Simulator before a real PLC,
- ONNX Runtime before TensorRT,
- CLI/headless execution before a complete Studio,
- local deployment before Hub-managed deployment.

## 3.5 One Production Result Per Trigger

Every accepted trigger must end in exactly one explicit final status:

```text
OK
NG
ERROR
NO_RESULT
TIMEOUT
INVALID
BYPASS
```

Technical failure must never be converted to `OK` or ordinary `NG`.

---

# 4. Target Product Outcome

The first complete NVision release must allow a vision engineer to:

1. Open or create an NVision Project.
2. Configure a logical camera.
3. Create a Job and node-based Workflow.
4. Import a versioned `.nvmodel` package.
5. Create a product Recipe.
6. Test the Workflow offline using File Camera.
7. Publish an immutable `.nvpkg` deployment.
8. Activate that deployment in Runtime.
9. Receive a machine or simulated trigger.
10. Acquire an image.
11. Execute the same Engine used by Studio test mode.
12. Produce explicit `OK`, `NG`, or `ERROR`.
13. Publish a compact PLC result.
14. Persist the full inspection evidence.
15. Recover safely from ordinary camera, PLC, model, or process faults.

No customer-specific application fork should be required for this scenario.

---

# 5. Canonical Repository Architecture

## 5.1 Target Structure

```text
nvision/
|
|-- apps/
|   |-- nvision-cli/
|   |-- nvision-studio/
|   |-- nvision-runtime/
|   |-- nvision-trainer/
|   |-- nvision-hub/
|   `-- shared/
|
|-- engine/
|   |-- core/
|   |-- acquisition/
|   |-- vision/
|   |-- ai/
|   |-- workflow/
|   |-- inspection/
|   |-- calibration/
|   |-- devices/
|   |-- communication/
|   |-- data/
|   `-- diagnostics/
|
|-- sdk/
|   |-- dotnet/
|   |-- cpp/
|   `-- python/
|
|-- plugins/
|   |-- abstractions/
|   |-- cameras/
|   |-- vision/
|   |-- ai/
|   |-- plc/
|   |-- robot/
|   `-- storage/
|
|-- schemas/
|   |-- project/
|   |-- recipe/
|   |-- model/
|   |-- result/
|   `-- plugin/
|
|-- tests/
|   |-- unit/
|   |-- integration/
|   |-- architecture/
|   |-- compatibility/
|   |-- golden/
|   |-- hardware/
|   |-- performance/
|   `-- system/
|
|-- samples/
|   `-- wire-marker/
|
|-- tools/
|-- docs/
|-- tasks/
|-- handoff/
|-- decisions/
|-- scripts/
|-- artifacts/
|
|-- Directory.Build.props
|-- Directory.Packages.props
|-- global.json
|-- .editorconfig
|-- .gitignore
|-- AGENTS.md
|-- CLAUDE.md
|-- NTech.NVision.sln
`-- README.md
```

## 5.2 Folder Responsibilities

| Folder | Responsibility |
|---|---|
| `apps` | Executable products and composition roots |
| `engine` | Shared platform behavior used by CLI, Studio, Runtime, SDK, and Edge |
| `sdk` | Public embedding and remote-control surfaces |
| `plugins` | Replaceable vendor, backend, node, protocol, and storage implementations |
| `schemas` | Durable, versioned serialized contracts |
| `tests` | Quality validation organized by test purpose |
| `samples` | Executable reference applications and regression fixtures |
| `tools` | Build, migration, package, diagnostics, and developer utilities |
| `docs` | Product, architecture, design, device, protocol, SDK, and decision knowledge |

## 5.3 Creation Policy

The target structure is a direction, not permission to create empty assemblies.

Create a project only when it has:

- a defined responsibility,
- an explicit dependency boundary,
- at least one consumer,
- and a test or executable acceptance path.

Trainer, Hub, C++ SDK, Python SDK, robot plugins, and additional vendors remain documentation-only until their roadmap phase begins.

---

# 6. Documentation Reorganization

The existing foundation documents must move with Git history preserved.

```text
docs/
|-- README.md
|-- product/
|   |-- NVISION_PLATFORM_FOUNDATION.md
|   `-- 12_PRODUCT_ROADMAP.md
|-- architecture/
|   |-- 01_PLATFORM_ARCHITECTURE.md
|   |-- 02_DOMAIN_MODEL.md
|   |-- 03_WORKFLOW_ENGINE_ARCHITECTURE.md
|   |-- 04_PLUGIN_ARCHITECTURE.md
|   |-- 05_PROJECT_FILE_FORMAT.md
|   |-- 06_INSPECTION_RESULT_SCHEMA.md
|   |-- 09_RUNTIME_ARCHITECTURE.md
|   |-- 10_STUDIO_UI_ARCHITECTURE.md
|   `-- 14_REPOSITORY_STRUCTURE.md
|-- design/
|   `-- 11_DESIGN_SYSTEM.md
|-- ai/
|   |-- 08_AI_ENGINE_ARCHITECTURE.md
|   `-- MODEL_FORMAT.md
|-- devices/
|   |-- 07_CAMERA_ABSTRACTION.md
|   |-- PLC_INTEGRATION.md
|   `-- ROBOT_INTEGRATION.md
|-- protocols/
|   `-- 17_PLC_HANDSHAKE_SPEC.md
|-- sdk/
|-- decisions/
|-- implementation/
|   `-- 13_IMPLEMENTATION_PLAN.md
|-- testing/
|   `-- 18_TEST_STRATEGY.md
`-- deployment/
    `-- 19_DEPLOYMENT_GUIDE.md
```

Rules:

- Use `git mv`; do not copy and leave duplicate source-of-truth files.
- Preserve numeric prefixes for the foundation reading order.
- Add a documentation index with purpose, status, owner, and reading order.
- Keep operational repository context separate from product documentation.
- Store actual ADRs under `docs/decisions/`.
- Keep `decisions/0000_INDEX.md` as the repository-workflow index pointing to the ADRs.

---

# 7. Workspace Integration

The code repository remains:

```text
06_Repositories/NTech/
```

Create project-private working context at:

```text
05_Projects/NTech/nvision/
|-- README.md
|-- context.md
|-- active_task.md
|-- task_board.md
|-- backlog.md
|-- blocked.md
|-- done.md
|-- handoff.md
|-- session_history.md
|-- decisions_private.md
|-- ai_workflow.md
`-- external_backup.md
```

Rules:

- Source code and shared technical documentation belong only in `06_Repositories/NTech`.
- Personal planning and handoff belong in `05_Projects/NTech/nvision`.
- Do not duplicate the detailed backlog in workspace-level task management.
- Do not copy NVision source into RoboStation or another repository.
- RoboStation may be used as a repository-operating reference, not as NVision's architecture template.

---

# 8. Technology Baseline

## 8.1 Initial Platform

Recommended baseline for the current development environment:

| Area | Choice |
|---|---|
| Language | C# |
| SDK/runtime | .NET 10, pinned by `global.json` |
| Cross-platform libraries | `net10.0` |
| Windows UI | `net10.0-windows` + WPF |
| UI pattern | MVVM |
| Hosting | .NET Generic Host |
| Serialization | `System.Text.Json` |
| AI reference backend | ONNX Runtime |
| Vision | OpenCvSharp, introduced only when required by the first nodes |
| Local database | SQLite |
| Logging | Microsoft logging abstractions + Serilog provider |
| Runtime control API | gRPC |
| Tests | xUnit |
| Package management | Central Package Management |

If production hosts require .NET 8, change the target during P0 and record it in an ADR. Do not support multiple target frameworks in the first slice without a concrete deployment requirement.

## 8.2 Package Policy

- Lock package versions centrally.
- Do not add a package without a direct first-slice use.
- Keep vendor SDK packages inside their plugin projects.
- Keep WPF packages inside application presentation projects.
- Keep ONNX Runtime types inside the ONNX plugin.
- Keep SQLite implementation types inside storage/data implementation projects.
- Document native runtime requirements and license information.

---

# 9. Initial Solution and Project Map

## 9.1 P0–P2 Projects

Create first:

```text
engine/core/NTech.NVision.Core
engine/core/NTech.NVision.Domain
engine/core/NTech.NVision.Contracts
engine/core/NTech.NVision.Application
engine/workflow/NTech.NVision.Engine.Workflow
engine/acquisition/NTech.NVision.Engine.Acquisition
engine/ai/NTech.NVision.Engine.AI
engine/inspection/NTech.NVision.Engine.Inspection
engine/data/NTech.NVision.Engine.Data
plugins/abstractions/NTech.NVision.Plugin.Abstractions
plugins/vision/builtin/NTech.NVision.Nodes.BuiltIn
plugins/cameras/file/NTech.NVision.Plugin.Camera.File
plugins/ai/onnx-runtime/NTech.NVision.Plugin.AI.OnnxRuntime
plugins/plc/simulator/NTech.NVision.Plugin.Plc.Simulator
plugins/storage/sqlite/NTech.NVision.Plugin.Storage.Sqlite
plugins/storage/file-system/NTech.NVision.Plugin.Storage.FileSystem
apps/nvision-cli/NTech.NVision.Cli
```

Do not create Studio, Runtime UI, Trainer, Hub, real camera, real PLC, or TensorRT projects in P0.

## 9.2 P3 Projects

Add when ProjectFormat and Studio work begins:

```text
engine/core/NTech.NVision.ProjectFormat
apps/shared/NTech.NVision.UI
apps/nvision-studio/NTech.NVision.Studio
apps/nvision-studio/NTech.NVision.Studio.Application
apps/nvision-studio/NTech.NVision.Studio.Presentation
```

## 9.3 P4 Projects

Add when production Runtime begins:

```text
apps/nvision-runtime/NTech.NVision.Runtime.Service
apps/nvision-runtime/NTech.NVision.Runtime.Contracts
apps/nvision-runtime/NTech.NVision.Runtime.UI
```

## 9.4 P5+ Projects

Add only after selecting actual hardware:

```text
plugins/cameras/<first-vendor>/NTech.NVision.Plugin.Camera.<Vendor>
plugins/plc/<first-protocol>/NTech.NVision.Plugin.Plc.<Protocol>
plugins/ai/tensorrt/NTech.NVision.Plugin.AI.TensorRT
```

---

# 10. Dependency Rules

## 10.1 Allowed Direction

```text
Core
  ^
Domain <---- Contracts
  ^             ^
Workflow        Plugin Abstractions
  ^             ^
Inspection -----+
  ^
Application
  ^
CLI / Studio / Runtime Service
```

Concrete plugins depend inward on abstractions. Core projects never depend on concrete plugins.

## 10.2 Mandatory Prohibitions

- Domain must not reference WPF.
- Domain must not reference OpenCV.
- Domain must not reference ONNX Runtime or TensorRT.
- Domain must not reference camera or PLC SDKs.
- Engine must not reference Studio or Runtime UI.
- Studio ViewModels must not reference vendor SDK types.
- Workflow node contracts must not expose backend-specific result types.
- Runtime UI must not own production state.
- A plugin must not control Runtime lifecycle.
- Project source must not contain transient runtime device state.

## 10.3 Architecture Tests

Automated architecture tests must enforce:

- forbidden assembly references,
- no circular project references,
- namespace ownership,
- UI isolation,
- vendor SDK isolation,
- and plugin-to-core dependency direction.

---

# 11. Core Domain Implementation Scope

## 11.1 Common Types

```text
Result<T>
DomainError
ValidationIssue
SemanticVersion
Revision
Timestamp/clock abstraction
```

## 11.2 Strong Identities

```text
ProjectId
JobId
WorkflowId
NodeId
NodeTypeId
RecipeId
DeviceId
ModelId
CalibrationId
InspectionId
DeploymentId
PluginId
```

## 11.3 Phase 1 Aggregates and Entities

```text
Project
Job
WorkflowDefinition
WorkflowNode
WorkflowConnection
Recipe
RecipeOverride
DeviceConfiguration
AIModelDefinition
Inspection
InspectionResult
DeploymentPackage
```

## 11.4 First-Slice Value Objects

```text
PortDefinition
PortDataType
NodeParameterValue
ProductIdentity
TriggerEvent
Measurement
Confidence
ClassificationResult
Detection
BoundingBox
TextResult
ImageReference
InspectionTiming
InspectionReason
InspectionTraceability
```

## 11.5 Domain Invariants

- IDs are stable and explicit.
- Published Project and Recipe snapshots are immutable.
- Runtime state is not authoring configuration.
- Workflow connections reference existing typed ports.
- Recipe overrides reference valid overrideable parameters.
- Final InspectionResult is immutable.
- Final status and decision code are compatible.
- `OK` cannot contain a blocking technical error.
- Measurements include units.
- Model, Recipe, Project, Deployment, and plugin versions are traceable.

---

# 12. Initial Contracts and Responsibilities

These names are implementation targets, not complete code signatures.

## 12.1 Workflow

```text
INodeTypeRegistry
INodeExecutor
IWorkflowValidator
IWorkflowCompiler
IWorkflowRuntime
IResourceLockManager
IExecutionTraceSink
```

Primary classes:

```text
NodeTypeRegistry
WorkflowValidator
ExecutionPlanCompiler
ExecutionPlan
CompiledNode
WorkflowScheduler
WorkflowExecutionContext
PortValueStore
WorkflowExecutionResult
```

## 12.2 Acquisition and Camera

```text
ICameraDriverFactory
ICameraDevice
ICameraRegistry
IAcquisitionService
```

Primary types:

```text
CameraConfiguration
CameraCapabilities
CameraState
GrabRequest
Frame
FrameLease
FrameMetadata
PixelFormat
CameraError
```

## 12.3 AI

```text
IAIBackendFactory
IAIModelSession
IAIInferenceService
IModelRegistry
IModelSlotResolver
```

Primary classes:

```text
ModelPackageLoader
ModelManifestValidator
ModelRegistry
ModelSlotResolver
AIBackendRegistry
AIModelSessionManager
ClassificationPreprocessor
ClassificationPostprocessor
```

## 12.4 Inspection

```text
IInspectionEngine
IInspectionResultBuilder
IInspectionResultValidator
IInspectionIdGenerator
```

Primary classes:

```text
InspectionContext
InspectionRequest
InspectionEngine
InspectionResultBuilder
InspectionResultValidator
InspectionExecutionResult
```

## 12.5 Persistence

```text
IInspectionRepository
IImageStore
IDeploymentStore
IProjectRepository
IOutboxRepository
```

## 12.6 Plugin System

```text
INVisionPlugin
IPluginRegistry
IPluginLoader
IPluginRegistrationContext
```

Primary classes:

```text
PluginManifest
PluginDescriptor
PluginDirectoryScanner
PluginManifestValidator
PluginCompatibilityChecker
PluginRegistry
PluginLoader
PluginDiagnostics
```

---

# 13. Workflow Execution Model

The Workflow Engine must follow this lifecycle:

```text
WorkflowDefinition
    -> schema validation
    -> graph validation
    -> typed-port validation
    -> parameter/reference validation
    -> Recipe resolution
    -> immutable ExecutionPlan compilation
    -> bounded execution
    -> WorkflowExecutionResult
```

Initial execution requirements:

- directed acyclic graph,
- explicit typed ports,
- deterministic topological scheduling,
- sequential execution first,
- bounded parallel branches after sequential correctness,
- workflow and node timeout,
- cancellation propagation,
- explicit failure policies,
- resource cleanup,
- timing per node,
- repeatable execution of a compiled plan,
- no UI or hardware SDK dependency.

Initial built-in nodes:

```text
ImageInput
Crop
Resize
ColorConvert
Classify
Compare
AND
OR
NOT
Decision
SaveImage
```

Additional traditional vision nodes are deferred until the first slice is stable.

---

# 14. Project and Package Formats

## 14.1 Authoring Project

```text
WireMarkerInspection/
|-- WireMarkerInspection.nvision
|-- jobs/
|   `-- MarkerTypeInspection.njob
|-- recipes/
|   |-- TYPE_01.nrecipe
|   `-- TYPE_04.nrecipe
|-- devices/
|   |-- devices.json
|   `-- plc-mapping.json
|-- models/
|   `-- models.json
|-- calibration/
|   `-- calibration.json
|-- resources/
|-- artifacts/
|-- authoring/
|   `-- layout.json
`-- .nvision/
    |-- cache/
    `-- autosave/
```

## 14.2 Authoring Rules

- UTF-8 without BOM.
- Deterministic property ordering.
- Two-space JSON indentation.
- Relative paths only.
- Stable IDs.
- Explicit `schemaVersion`.
- Atomic saves.
- Runtime state excluded.
- UI-local layout separated from execution configuration.
- Explicit migrations; no silent incompatible conversion.

## 14.3 Model Package

A production `.nvmodel` includes:

```text
identity
version
task
portable model artifact
input specification
output semantics
preprocessing
postprocessing
labels
checksum
compatibility metadata
```

## 14.4 Deployment Package

The publisher validates authoring source and produces immutable `.nvpkg` containing:

```text
Project snapshot
Jobs and Workflows
Recipes
Device configuration
Model packages/references
Calibration records
Exact plugin lock
Schema versions
Integrity hashes
Deployment manifest
```

Runtime consumes `.nvpkg`, not editable Project source.

---

# 15. Inspection Result and Storage

## 15.1 Canonical Result

Minimum result content:

```text
SchemaVersion
InspectionId
CorrelationId
Station
Product
Project
Job
Recipe
Status
DecisionCode
Reasons
Measurements
Classifications
Detections
TextResults
Timings
ImageReferences
NodeResults
Traceability
Errors
Warnings
StartedAt
CompletedAt
```

## 15.2 Storage Strategy

SQLite stores:

- indexed summary columns,
- canonical result JSON,
- outbox records when needed,
- alarm and runtime metadata as later phases require.

File storage stores:

- source images according to policy,
- NG images,
- diagnostic images,
- thumbnails if required,
- integrity and retention metadata.

Images are referenced from the result; they are not embedded in result JSON.

## 15.3 Persistence Priority

The first production policy must explicitly decide the ordering of:

```text
machine response
durable persistence
```

For the simulator slice, persist the result before reporting completion. Real machine policy must be confirmed in the PLC handshake specification.

---

# 16. First Executable Vertical Slice

## 16.1 Reference Application

Use Wire Marker Inspection with Recipes `TYPE_01` and `TYPE_04`.

## 16.2 Runtime Flow

```text
CLI or PLC Simulator trigger
    -> generate InspectionId
    -> resolve Project/Job/Recipe
    -> acquire image from File Camera
    -> execute Crop
    -> execute ONNX Classify
    -> compare expected product type
    -> finalize OK/NG/ERROR
    -> persist SQLite result
    -> store image according to policy
    -> publish summary
```

## 16.3 Required Demonstration

```text
TYPE_01 image + TYPE_01 Recipe
    -> OK

TYPE_04 image + TYPE_01 Recipe
    -> NG
    -> reason WRONG_TYPE

missing model / invalid image / node failure
    -> ERROR
    -> never OK or ordinary NG
```

## 16.4 CLI Surface

Planned commands:

```text
nvision validate <project-or-package>
nvision inspect <project-or-package> --recipe <code> --input <image>
nvision show-result <inspection-id>
nvision plugins list
nvision models validate <model-package>
```

The CLI is an engineering harness. It must use the same Application and Engine services later hosted by Runtime and Studio test mode.

---

# 17. Implementation Phases

# 17.1 P0 — Repository and Foundation

Objective:

> Produce a clean repository and solution whose architectural dependency rules are executable and testable.

Work:

1. Create a feature branch.
2. Bootstrap repository-local AI workflow files.
3. Reorganize documentation using `git mv`.
4. Create documentation index.
5. Create the canonical top-level folder structure needed for P0–P2.
6. Create solution and initial projects.
7. Add `global.json`.
8. Add central package management.
9. Add shared build properties.
10. Add editor and analyzer configuration.
11. Add build/test/check scripts.
12. Add CI pipeline.
13. Add the first unit test.
14. Add architecture tests.
15. Record foundational ADRs.

Demo:

```text
clean checkout
    -> restore
    -> build
    -> test
    -> architecture checks pass
```

Exit gate:

- solution builds cleanly,
- CI passes,
- no circular reference,
- Domain has no forbidden dependencies,
- Engine has no UI dependency,
- no vendor SDK exists outside a plugin.

# 17.2 P1 — Core Domain and Workflow Engine

Objective:

> Validate the domain and execution architecture without images, UI, AI, database, or hardware.

Work:

1. Implement strong IDs.
2. Implement Result/error/validation primitives.
3. Implement Project and Job.
4. Implement WorkflowDefinition, Node, Port, Connection.
5. Implement Recipe and override resolution.
6. Implement node metadata and registry.
7. Implement validation pipeline.
8. Implement immutable ExecutionPlan.
9. Implement sequential scheduler.
10. Implement fake deterministic nodes.
11. Implement failure propagation.
12. Implement cancellation and timeout.
13. Implement node timing and trace.
14. Add bounded parallel branches.
15. Add CLI workflow harness.

Implementation status (2026-08-19): items 1–15 are complete. The MVP runtime deliberately supports `AbortWorkflow` and `Continue`; `ReturnNG` belongs to the Inspection/product-result layer, while fallback should be represented by explicit graph structure. Bounded parallel scheduling uses a stable ready queue, dependency counters, and one configurable overall concurrency limit; resource-class queues and keyed device locks are intentionally deferred.

Demo:

```text
Create definition
    -> validate
    -> compile
    -> execute fake graph
    -> deterministic decision
```

Exit gate:

- invalid cycle rejected,
- type mismatch rejected,
- failed node handled explicitly,
- timeout works,
- cancellation works,
- repeated plan execution is stable,
- no UI/vendor dependency.

# 17.3 P2 — Wire Marker Headless Vertical Slice

Objective:

> Run one complete image inspection through the real reference AI backend and persist the evidence.

Work:

1. Implement image/frame contracts and ownership.
2. Implement File Camera.
3. Implement AcquisitionService.
4. Implement `.nvmodel` manifest and loader.
5. Implement model registry and slot resolution.
6. Implement ONNX Runtime plugin.
7. Implement CPU reference preprocessing.
8. Implement classification postprocessing.
9. Implement Crop, Classify, Compare, and Decision nodes.
10. Implement Inspection lifecycle.
11. Implement InspectionResult builder and validator.
12. Implement SQLite result persistence.
13. Implement file image storage.
14. Create Wire Marker fixtures.
15. Create TYPE_01 and TYPE_04 configurations.
16. Add golden tests.
17. Add end-to-end CLI demo.

Demo:

```text
type01.png + TYPE_01 -> OK
type04.png + TYPE_01 -> NG WRONG_TYPE
model failure -> ERROR
```

Exit gate:

- exact P2 demo passes,
- result is immutable,
- Project/Recipe/Model/plugin/backend versions are recorded,
- SQLite history can reconstruct the result,
- image lifetime has no leak in repeat tests,
- technical errors are never mapped to valid quality decisions.

Implementation status (2026-08-19): **Complete**.

The executable reference uses text-reviewable 8×8 PGM fixtures and a directory-backed development `.nvmodel`. Its Base64-encoded ONNX artifact is checksum-validated and executed by the actual ONNX Runtime CPU plugin. This intentionally small deterministic model proves the backend/package/traceability boundary; it is not a production-trained classifier. Golden and repeat tests cover OK, WRONG_TYPE, technical ERROR, SQLite reconstruction, and frame ownership.

# 17.4 P3 — ProjectFormat and Studio MVP

Objective:

> Author, test, save, reopen, validate, and publish the Wire Marker Project without editing source code.

Order:

1. ProjectFormat DTOs and common headers.
2. Root `.nvision` manifest.
3. `.njob` and `.nrecipe`.
4. Device, model, and calibration references.
5. Loader and cross-reference resolver.
6. Canonical writer and atomic save.
7. Migration infrastructure.
8. Publisher and `.nvpkg`.
9. Shared UI tokens and primitive components.
10. WPF Studio shell.
11. Project Explorer.
12. Workflow canvas.
13. Property Inspector.
14. Image canvas.
15. Undo/Redo.
16. Recipe editor.
17. Minimal model manager.
18. Test runner using the same Engine.
19. Validation summary.
20. Publish workflow.

Demo:

```text
Open Wire Marker
    -> change threshold
    -> run test
    -> save
    -> reopen
    -> run again
    -> publish .nvpkg
```

Exit gate:

- stable save/load round trip,
- typed-port authoring,
- undo/redo works,
- Studio test matches headless result,
- published package is immutable and integrity checked,
- WPF types remain in Presentation.

# 17.5 P4 — Runtime MVP

Objective:

> Run the published Project as an independent production authority.

Work:

1. Create headless Runtime Service.
2. Implement staged startup.
3. Implement Runtime state machine.
4. Implement deployment validation and activation.
5. Implement known-good deployment tracking.
6. Load required plugins.
7. Resolve Recipe and compile Workflow.
8. Load and warm active models.
9. Implement readiness evaluation.
10. Implement PLC Simulator handshake.
11. Implement trigger-to-inspection pipeline.
12. Implement runtime event stream.
13. Implement health/readiness/control gRPC API.
14. Implement minimal Runtime UI as a separate client.
15. Implement graceful stop.
16. Implement process restart recovery.

Demo:

```text
publish
    -> start Runtime Service
    -> activate package
    -> Runtime READY
    -> PLC Simulator trigger
    -> Busy
    -> OK/NG/ERROR
    -> persisted result
    -> Ack
    -> next trigger
```

Exit gate:

- Service runs without UI,
- UI can restart independently,
- one trigger produces one result,
- invalid deployment preserves known-good deployment,
- invalid Recipe activation preserves current Recipe,
- AI failure does not crash the Service,
- graceful shutdown does not accept unsafe new work.

# 17.6 P5 — First Real Hardware

Objective:

> Replace simulation boundaries independently without modifying core execution logic.

Work:

1. Select one camera vendor from actual project requirements.
2. Implement camera plugin discovery and binding.
3. Implement capability reporting.
4. Implement exposure, gain, pixel format, and trigger configuration.
5. Implement readback and effective configuration.
6. Implement single grab and preview.
7. Implement timeout, incomplete-frame detection, disconnect, and reconnect.
8. Add camera contract tests and soak test.
9. Select one PLC protocol.
10. Implement typed read/write and address validation.
11. Implement production handshake.
12. Add HIL machine-sequence tests.

Demo:

```text
real trigger
    -> real camera frame
    -> same Wire Marker Workflow
    -> real PLC result
```

Exit gate:

- File Camera and real camera are interchangeable,
- PLC Simulator and real PLC are interchangeable,
- vendor SDK types remain inside plugins,
- reconnect requires configuration revalidation,
- hardware faults create explicit Runtime state.

# 17.7 P6 — AI Acceleration

Objective:

> Optimize only after correctness and traceability are established.

Work:

- TensorRT backend plugin,
- engine build/cache tool,
- backend equivalence tests,
- precision and fallback policy,
- GPU memory diagnostics,
- p50/p95/p99 performance measurement,
- optional OpenVINO only for a concrete target.

Exit gate:

- same Workflow and AI result contracts,
- backend selection is configuration-controlled,
- actual backend and precision are recorded,
- fallback is explicit,
- performance target is measured on target hardware.

# 17.8 P7 — Production Hardening

Objective:

> Make Runtime safe for unattended production operation.

Work:

- camera and PLC recovery,
- AI session recovery,
- disk-low and disk-full policy,
- retention worker,
- durable outbox,
- watchdog and crash recovery,
- known-good rollback,
- atomic Recipe switch,
- alarms,
- audit,
- structured diagnostics,
- support bundle,
- API authentication and roles,
- service installation,
- fault injection,
- power-cycle tests,
- 72-hour soak test.

Exit gate:

- no known critical memory leak,
- no silent result loss,
- no reconnect deadlock,
- no deployment corruption,
- recovery and rollback demonstrations pass,
- support engineer can diagnose from InspectionId and support bundle.

# 17.9 P8 — Advanced Vision

Add only after P7:

- traditional vision catalog,
- detection,
- segmentation,
- OCR,
- anomaly detection,
- multi-camera,
- calibration,
- Gear Inspection reference application.

# 17.10 P9–P12 — Ecosystem Expansion

Deferred roadmap:

```text
P9  Edge / Jetson
P10 Trainer
P11 Hub
P12 SDK and ecosystem
```

These phases must not expand P0–P7 scope unless a real customer or deployment requirement changes the priority.

---

# 18. Test Strategy

## 18.1 Unit Tests

Cover:

- domain invariants,
- strong IDs and value objects,
- Recipe resolution,
- graph validation,
- compiler behavior,
- failure policies,
- result validation,
- serialization converters.

## 18.2 Architecture Tests

Cover dependency and namespace rules from Section 10.

## 18.3 Integration Tests

Cover:

- Workflow runtime with fake services,
- File Camera acquisition,
- ONNX inference,
- SQLite persistence,
- plugin discovery and compatibility,
- project load/save/publish,
- Runtime startup and trigger flow.

## 18.4 Golden Tests

Retain fixed inputs and expected outputs for:

- Wire Marker classifications,
- preprocessing,
- result JSON,
- ProjectFormat versions,
- model backend equivalence.

## 18.5 Compatibility Tests

Retain fixtures for every released:

- Project schema,
- Recipe schema,
- result schema,
- model manifest,
- plugin manifest.

## 18.6 Hardware Tests

Run contract tests against each certified camera and PLC combination.

## 18.7 Performance Tests

Measure:

- acquisition time,
- preprocessing,
- inference,
- workflow total,
- persistence latency,
- p50/p95/p99 cycle time,
- memory stability.

## 18.8 System and Soak Tests

Include:

- camera unplug/reconnect,
- PLC disconnect,
- model load failure,
- Runtime restart,
- UI restart,
- disk low/full,
- invalid Recipe,
- invalid deployment,
- power cycle,
- repeated inspection,
- 72-hour continuous operation.

---

# 19. CI and Engineering Workflow

Every pull request must run:

```text
restore
build
unit tests
architecture tests
format/analyzer checks
schema tests
relevant integration tests
```

Later pipelines add:

```text
golden tests
package/publish tests
Windows WPF build
native backend matrix
hardware certification jobs
performance baselines
```

Required scripts:

```text
scripts/restore.ps1
scripts/build.ps1
scripts/test.ps1
scripts/check.ps1
scripts/run-wire-marker-demo.ps1
scripts/publish-runtime.ps1
scripts/package.ps1
```

Scripts must be non-interactive by default and fail on the first required step that fails.

---

# 20. Initial ADR Set

Create and approve:

```text
ADR-0001 Platform-first monorepo
ADR-0002 Capability-oriented repository root
ADR-0003 Modular monolith first
ADR-0004 Runtime separate from Studio
ADR-0005 Node-based typed Workflow
ADR-0006 In-process plugins for MVP
ADR-0007 ONNX as initial model exchange
ADR-0008 ONNX Runtime as reference backend
ADR-0009 SQLite plus file storage
ADR-0010 WPF for initial Studio and Runtime UI
ADR-0011 gRPC Runtime control API
ADR-0012 .NET target framework baseline
ADR-0013 Canonical JSON and explicit schema migration
ADR-0014 Immutable production deployment package
```

---

# 21. First 50 Engineering Tasks

## Foundation

1. Create implementation branch.
2. Bootstrap repo workflow scaffold.
3. Move documentation into canonical folders.
4. Create documentation index.
5. Create project-private workspace context.
6. Create root repository structure.
7. Create solution.
8. Add SDK pinning and central package management.
9. Add shared build/analyzer rules.
10. Add CI and check scripts.

## Domain and Workflow

11. Add strong IDs.
12. Add Result/error/validation primitives.
13. Add Project and Job.
14. Add WorkflowDefinition.
15. Add node and typed-port descriptors.
16. Add connections and graph invariants.
17. Add Recipe and parameter overrides.
18. Add node registry.
19. Add Workflow validation.
20. Add ExecutionPlan compiler.
21. Add sequential Workflow runtime.
22. Add failure propagation.
23. Add cancellation and timeout.
24. Add timing and execution trace.
25. Add bounded parallel scheduling.

## Image, AI, and Inspection

26. Add image/frame/pixel format contracts.
27. Add FrameLease and ownership tests.
28. Add File Camera plugin.
29. Add AcquisitionService.
30. Add InspectionStatus and InspectionResult.
31. Add Inspection engine and result builder.
32. Add `.nvmodel` manifest.
33. Add model package validation and checksum.
34. Add model registry and slot resolver.
35. Add ONNX Runtime backend.
36. Add classification preprocessing.
37. Add classification postprocessing.
38. Add Crop node.
39. Add Classify node.
40. Add Compare and Decision nodes.

## Reference Slice and Persistence

41. Create Wire Marker fixtures.
42. Create TYPE_01 Recipe.
43. Create TYPE_04 Recipe.
44. Add golden classification tests.
45. Add SQLite result repository.
46. Add file image store.
47. Add CLI validate command.
48. Add CLI inspect command.
49. Add end-to-end Wire Marker test.
50. Add repeatable P2 demo script and document the result.

ProjectFormat, Studio, Runtime Service, real hardware, and hardening tasks begin only after Task 50 passes its exit gate.

---

# 22. Release Milestones

```text
M0 Foundation builds
M1 Pure Workflow executes
M2 Wire Marker runs headless
M3 Studio authors and publishes
M4 Runtime simulation runs independently
M5 Real camera works
M6 Real PLC handshake works
M7 Production pilot passes
M8 Production 1.0 passes hardening gates
M9 Advanced Vision
M10 Edge
M11 Trainer
M12 Hub and SDK ecosystem
```

Use capability and exit gates, not arbitrary calendar dates.

---

# 23. Definition of Done

A feature is done only when applicable items are complete:

- behavior implemented,
- unit/integration tests added,
- error codes and failure behavior defined,
- cancellation and timeout implemented for external or long operations,
- logging and diagnostics added,
- traceability recorded,
- configuration validation added,
- schema/version impact handled,
- documentation updated,
- demo or acceptance path updated,
- build and checks pass.

A feature without error handling is not done.

A long-running or external-I/O feature without cancellation and timeout is not done.

An execution-relevant change without version traceability is not done.

An authoring feature without validation is not done.

---

# 24. Production 1.0 Mandatory Gates

## Architecture

- no vendor SDK in Domain or ViewModels,
- headless Runtime,
- same Engine for Studio and Runtime,
- versioned plugins,
- immutable compiled execution plans.

## Project and Deployment

- stable save/load,
- explicit schema versions,
- immutable `.nvpkg`,
- integrity validation,
- known-good rollback.

## Workflow

- validation,
- compilation,
- typed ports,
- timeout,
- cancellation,
- failure propagation,
- deterministic result.

## Camera and AI

- one real camera,
- timeout and incomplete-frame detection,
- reconnect and configuration readback,
- versioned model package,
- warm-up,
- golden AI tests,
- explicit AI errors.

## Runtime and PLC

- readiness and fault states,
- controlled handshake,
- crash recovery,
- graceful stop,
- atomic Recipe switch,
- one final result per trigger.

## Data and Support

- SQLite history,
- image retention,
- disk monitoring,
- backup/restore,
- structured logs,
- alarm codes,
- support bundle,
- full version traceability.

## Quality

- unit, integration, golden, HIL, fault-injection, and soak tests,
- FAT and SAT procedures,
- no known critical memory leak, result loss, reconnect deadlock, or deployment corruption.

---

# 25. Risks and Controls

| Risk | Control |
|---|---|
| Architecture overengineering | Create projects only with a consumer and test; modular monolith first |
| Scope explosion | Wire Marker gate before broad features; defer Hub/Trainer/marketplace |
| Workflow editor complexity | Prove headless Workflow first; simple canvas MVP |
| Image lifetime leaks | Explicit FrameLease, pooling rules, repeat and leak tests |
| AI preprocess mismatch | Put semantics in `.nvmodel`; golden preprocessing/backend tests |
| Vendor lock-in | Plugin boundary; one vendor at a time; contract tests |
| PLC ambiguity | Simulator, explicit sequence/ACK/timeout specification |
| Traceability gaps | Immutable InspectionResult with exact versions and IDs |
| Runtime/UI coupling | Separate Runtime Service and UI process |
| Silent recovery | Explicit states, alarms, readiness revalidation |
| Documentation drift | Docs updated in feature DoD; ADR for contract changes |
| Empty repository scaffolding | Create only P0–P2 physical projects initially |

---

# 26. Decisions Required Before P5

The following do not block P0–P4 but must be resolved before real hardware:

- first camera vendor and SDK version,
- first PLC vendor/protocol,
- actual PLC handshake register/symbol map,
- target machine cycle time and p95/p99 limits,
- target Windows/IPC hardware,
- image retention and disk policy,
- required source/NG image evidence level,
- production authentication and user roles,
- model ownership and update procedure,
- installation and remote-support constraints.

---

# 27. Immediate Execution Sequence After Approval

The next implementation session should perform only P0:

```text
1. Create branch
2. Bootstrap repo workflow scaffold
3. Move and index documents
4. Create workspace project context
5. Create P0–P2 target folders
6. Create solution and minimal project set
7. Add build/package policy files
8. Add one unit test and architecture tests
9. Add CI/check scripts
10. Run restore, build, test, and checks
11. Review P0 exit gate
12. Update handoff and implementation status
```

Do not begin P1 implementation until P0 has a clean, reproducible build and the dependency rules are verified.

---

# 28. Final Implementation Statement

The NVision implementation strategy is:

> **Build the smallest complete industrial vision platform that runs one real reference inspection end to end, preserves strict platform boundaries and production evidence, and can replace simulation with real hardware without rewriting the core.**

The implementation order is therefore:

```text
Repository
    -> Domain
    -> Workflow
    -> Image ownership
    -> File Camera
    -> Inspection Result
    -> ONNX Classification
    -> Wire Marker headless slice
    -> SQLite and image storage
    -> ProjectFormat and publish
    -> Studio
    -> Runtime Service
    -> Real Camera
    -> Real PLC
    -> Production hardening
    -> Advanced platform capabilities
```

Every milestone ends with an executable demonstration and an explicit exit gate.

---

**End of Document**
