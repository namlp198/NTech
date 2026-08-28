# ADR-0006 — Core and Workflow ownership boundary in the native plane

- **Status:** Accepted
- **Date:** 2026-08-27
- **Supersedes:** none
- **Related:** ADR-0005, `docs/implementation/16_GENERIC_VISION_TOOL_PLATFORM_EXECUTION_PLAN.md` Section 13

## Context

The native plane grew milestone by milestone. N0/N1 established `native/core` and
`native/acquisition`. M1 added the generic data contracts to `native/core`. M2 then added
the tool catalog and the persisted workflow contracts — `ToolTypeDefinition`,
`ToolRegistry`, the parameter and property-editor schemas, `WorkflowNode`,
`WorkflowGraph`, and `SubworkflowDefinition` — to `native/core` as well, because no other
area existed yet.

Section 13 of the execution plan assigns that behavior elsewhere:

```text
core/       generic IDs, status, result, geometry, resources
workflow/   registry, validation, compile, scheduling, execution
```

M3 made the drift concrete. The tool execution foundation was added under
`native/workflow` and `nvision::workflow`, which left execution in one area while the
contracts it executes stayed in another. Every later milestone — M4 recognition, M5
workflow/job/recipe execution — adds code on both sides of that seam, so the cost of the
inconsistency grows with each slice.

## Decision

The native plane owns these areas with a strictly one-way dependency:

```text
nvision::core       data vocabulary
                    identifiers, semantic versions, port data kinds, geometry and
                    transforms, resource references, execution status, inspection
                    outcome, execution and inspection results
        ▲
        │
nvision::workflow   tool platform and execution
                    parameter and property-editor schemas, tool definitions, the tool
                    registry and built-in catalog, workflow nodes and graphs,
                    subworkflows, typed port values, the tool execution contract, and
                    the tool implementation registry
        ▲
        │
nvision::vision     generic image tool implementations
```

`nvision::acquisition` continues to depend on `nvision::core` only.

Core never includes a workflow header. A type belongs to core when it is a value
vocabulary that means the same thing outside any tool or graph. A type belongs to
workflow when it exists to describe, persist, validate, or run a tool or a graph.

## Consequences

- The seven file pairs `parameter_schema`, `tool_definition`, `tool_registry`,
  `builtin_catalog`, `workflow_node`, `workflow_graph`, and `subworkflow` moved from
  `native/core` to `native/workflow`, and their namespace changed from `nvision::core` to
  `nvision::workflow`. The move is mechanical: no behavior, contract, or test expectation
  changed.
- `nvision_workflow` is a separate static library that links `nvision_core` publicly.
  Native test targets that exercise the tool contracts link `nvision_workflow`.
- The versioned C ABI is unaffected. It consumes `nvision/core/engine.hpp` and the
  acquisition headers only, so no exported symbol, managed P/Invoke signature, or
  Engine Host contract changed.
- Callers outside the native plane are unaffected because these are internal C++ APIs.
  They remain unexposed through the C ABI and gRPC until M8.
- Doing this before M5 was deliberate. M5 fills `workflow/` with scheduling, compilation,
  and job/recipe execution, at which point relocating the contracts underneath it would be
  a materially larger and riskier change.

## Alternatives considered

- **Leave the split in place.** Rejected. Execution and the contracts it executes would
  stay in different areas indefinitely, and the plan's own ownership map would describe
  something the repository does not do.
- **Move the files but keep the `nvision::core` namespace.** Rejected. A header at
  `nvision/workflow/workflow_node.hpp` declaring `nvision::core::WorkflowNode` is more
  confusing than either consistent option.
- **Move execution into `nvision::core` instead.** Rejected. It contradicts Section 13 and
  would make core the place where OpenCV-era execution concerns accumulate.
