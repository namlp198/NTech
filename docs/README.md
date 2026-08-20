# NVision Documentation

This index is the source of truth for the NVision documentation set.

## Mandatory Reading Order

| Order | Document | Purpose | Status |
|---:|---|---|---|
| 0 | [Platform Foundation](product/NVISION_PLATFORM_FOUNDATION.md) | Product identity, principles, ecosystem, and baseline boundaries | Foundation |
| 1 | [Platform Architecture](architecture/01_PLATFORM_ARCHITECTURE.md) | Layers, process topology, dependencies, and technology direction | Foundation |
| 2 | [Domain Model](architecture/02_DOMAIN_MODEL.md) | Ubiquitous language, aggregates, invariants, and identities | Foundation |
| 3 | [Workflow Engine](architecture/03_WORKFLOW_ENGINE_ARCHITECTURE.md) | Validation, compilation, scheduling, errors, and execution | Foundation |
| 4 | [Plugin Architecture](architecture/04_PLUGIN_ARCHITECTURE.md) | Extension contracts, loading, versioning, and isolation | Foundation |
| 5 | [Project File Format](architecture/05_PROJECT_FILE_FORMAT.md) | `.nvision`, `.njob`, `.nrecipe`, migration, and publishing | Foundation |
| 6 | [Inspection Result Schema](architecture/06_INSPECTION_RESULT_SCHEMA.md) | Production evidence, traceability, and persistence | Foundation |
| 7 | [Camera Abstraction](devices/07_CAMERA_ABSTRACTION.md) | Logical cameras, acquisition, state, and frame ownership | Foundation |
| 8 | [AI Engine](ai/08_AI_ENGINE_ARCHITECTURE.md) | Model packages, backend abstraction, preprocessing, and inference | Foundation |
| 9 | [Runtime Architecture](architecture/09_RUNTIME_ARCHITECTURE.md) | Production authority, readiness, recovery, and machine coordination | Foundation |
| 10 | [Studio UI Architecture](architecture/10_STUDIO_UI_ARCHITECTURE.md) | Engineering authoring and test environment | Foundation |
| 11 | [Design System](design/11_DESIGN_SYSTEM.md) | Industrial UI tokens, components, and accessibility | Foundation |
| 12 | [Product Roadmap](product/12_PRODUCT_ROADMAP.md) | Dependency phases, milestones, and release gates | Foundation |
| 13 | [Implementation Plan](implementation/13_IMPLEMENTATION_PLAN.md) | Concrete repository, project, task, and delivery plan | Approved |
| 14 | [Repository Structure](architecture/14_REPOSITORY_STRUCTURE.md) | Canonical physical layout and ownership rules | Accepted |
| 15 | [Native Engine Architecture](architecture/15_NATIVE_ENGINE_ARCHITECTURE.md) | C++ execution plane, C ABI/P/Invoke, Engine Host, and memory transport | Accepted |

## Supporting Documentation

- [Repository context](00_CONTEXT.md)
- [AI-assisted workflow](README_AI_WORKFLOW.md)
- [Architecture decisions](decisions/)

Future device, protocol, testing, deployment, and SDK documents are added only when their implementation phase begins.

## Documentation Rules

- Preserve the numbered foundation reading order.
- Update architecture documents and ADRs when contracts change.
- Do not maintain duplicate source-of-truth copies.
- Keep runtime commands and operating procedures verifiable.
- Mark proposals, accepted decisions, deprecated content, and implemented behavior explicitly.
