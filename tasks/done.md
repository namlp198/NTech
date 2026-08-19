# Repository Done Tasks

| ID | Task | Completed Date | Evidence |
|---|---|---|---|
| P0-001 | Bootstrap repository workflow and project context | 2026-08-19 | `AGENTS.md`, `docs/00_CONTEXT.md`, `05_Projects/NTech/nvision/` |
| P0-002 | Reorganize and index foundation documents | 2026-08-19 | `docs/README.md` |
| P0-003 | Establish canonical capability-oriented root | 2026-08-19 | `docs/architecture/14_REPOSITORY_STRUCTURE.md` |
| P0-004 | Create .NET solution and dependency graph | 2026-08-19 | `NTech.NVision.sln` |
| P0-005 | Add architecture checks, scripts, and CI | 2026-08-19 | 8 architecture tests; `.github/workflows/ci.yml` |
| P1-001 | Implement Result/error/validation primitives | 2026-08-19 | 5 Core tests |
| P1-002 | Implement initial strong IDs | 2026-08-19 | 10 Domain tests |
| P1-003 | Implement Project aggregate and Job entity | 2026-08-19 | Project/Job domain tests |
| P1-004 | Implement WorkflowDefinition, nodes, and typed ports | 2026-08-19 | Typed graph domain model |
| P1-005 | Enforce connection and DAG invariants | 2026-08-19 | 20 new Domain tests; cycle/type mismatch rejection |
| P1-006 | Implement Recipe and typed parameter resolution | 2026-08-19 | Recipe precedence and validation tests |
| P1-007 | Implement node type metadata and registry | 2026-08-19 | Exact-version registry and origin tests |
| P1-008 | Implement staged Workflow validation | 2026-08-19 | Seven ordered stages; Engine.Workflow test project |
| P1-009 | Implement immutable ExecutionPlan compiler | 2026-08-19 | Stable topological plan, bound parameters, precomputed routing |
| P1-010 | Implement sequential Workflow runtime | 2026-08-19 | Typed execution scope and repeatable terminal output tests |
| P1-011 | Add fake deterministic nodes and CLI harness | 2026-08-19 | `(2 + 3) > 4` executes to `True` |
| P1-012 | Implement failure policies and dependency-aware propagation | 2026-08-19 | `AbortWorkflow`/`Continue`, contextual errors, skipped-node reasons |
| P1-013 | Implement Workflow/node timeout and cancellation hardening | 2026-08-19 | Linked cancellation hierarchy; non-cooperative executor tests |
| P1-014 | Add monotonic timing and structured execution trace | 2026-08-19 | Node/Workflow timing, trace events, non-fatal trace-sink warning |
| P1-015 | Implement bounded parallel scheduling | 2026-08-19 | Stable ready queue, dependency counters, configurable concurrency, parallel resilience tests |
| P1 | Complete Core Domain and Workflow Engine phase | 2026-08-19 | 99 tests; full repository gate; bounded-parallel CLI demo |
| P2-001 | Implement frame contracts and explicit ownership | 2026-08-19 | `Frame`, `IFrameBuffer`, `FrameLease`; ownership tests |
| P2-002 | Implement File Camera and AcquisitionService | 2026-08-19 | deterministic PGM replay; timeout/cancellation and late-lease tests |
| P2-003 | Implement `.nvmodel`, registry, and slot resolution | 2026-08-19 | manifest + checksum validation; immutable exact-version registry |
| P2-004 | Implement reference AI inference path | 2026-08-19 | ONNX Runtime CPU plugin; preprocessing and softmax/top-1 postprocessing |
| P2-005 | Implement Wire Marker Workflow nodes | 2026-08-19 | Crop/Classify/Compare/Decision through P1 compiler/runtime |
| P2-006 | Implement immutable result and evidence persistence | 2026-08-19 | result validator, file image store, SQLite reconstruction |
| P2-007 | Add Wire Marker fixtures, Recipes, golden tests, and CLI | 2026-08-19 | TYPE_01 OK; TYPE_04 WRONG_TYPE; backend ERROR; repeat demo script |
| P2 | Complete Wire Marker Headless Vertical Slice | 2026-08-19 | 113 tests; full repository gate; real ONNX Runtime CLI demo |
