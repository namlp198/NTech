# Native NVision Engine

This directory owns the canonical portable C++20 Core and Engine introduced by
ADR-0005.

```text
core/         Internal native Core/Engine implementation
acquisition/  Frame, native buffer ownership, File Camera, and acquisition lifecycle
abi/          Stable versioned C ABI for P/Invoke and embedding
host/         Production Engine Host gRPC contract and implementation
tests/        Native lifetime, ABI, behavior, and performance verification
```

Internal C++ APIs may evolve with the source tree. Only the C ABI and versioned gRPC
contract are binary/process boundaries. Do not export STL types, exceptions, raw vendor
types, or compiler-specific object layouts.

The N1 acquisition slice provides immutable native frames, shared ownership retained
until the final lease is released, deterministic ASCII PGM replay, bounded acquisition,
explicit cancellation, and lifecycle/error states. `nv_frame_get_buffer` is zero-copy;
its pointer remains valid only while the caller owns a frame handle.

The first M1 foundational-contract slice adds application-neutral stable identifiers,
semantic version precedence, generic native port data kinds, coordinate-space-aware
geometry and homographies, and an inspection outcome that prevents technical failures
from being represented as OK or NG. These are internal native contracts; C ABI exposure
is intentionally deferred until the generic boundary is proven.

`ImageView` adds bounded, read-only zero-copy access to a complete Frame. Full-frame,
ROI, and nested views retain the source Frame lease, expose stride-aware row spans, and
carry frame-specific coordinate spaces plus explicit view-to-source/source-to-view
transforms. Row spans remain valid only while the view or another Frame owner is alive.

Versioned `ResourceReference` contracts identify resource, model, template, calibration,
and dictionary content without embedding filesystem paths. Each reference carries an
exact semantic version and validated integrity metadata: canonical lowercase SHA-256 and
positive content length. Package/registry resolution and hash computation remain later
loader responsibilities.

Immutable `ExecutionRecord`, `InspectionDecision`, `EvidenceArtifact`, and
`InspectionResult` aggregates finalize the M1 contract layer. Successful execution may
produce OK, NG, or an explicit Indeterminate decision. Error, Timeout, and Cancelled
execution must remain Indeterminate, contain matching technical errors, and carry no
quality decision. Spatial evidence requires coordinate-space identity.

The first M2 slice adds immutable versioned `ToolTypeDefinition` contracts and a native
`ToolRegistry`. Definitions describe typed ports, hierarchical catalog metadata, origin,
and execution characteristics. Registry identity is only stable tool ID plus exact
semantic version; changing category metadata cannot change resolution identity. Missing
IDs and unsupported versions remain distinguishable unresolved states.

Declarative parameter schemas cover scalar values, enum options, recursively typed arrays,
coordinate-aware geometry, ROI shape constraints, and integrity-bearing versioned
resource/model/calibration references. Property editor metadata declares grouping, order,
control, and numeric units without UI dependencies. Default values, editor compatibility,
and conditional visibility dependency graphs are validated when definitions are finalized.

Persisted `WorkflowNode` instances complete the M2 contract layer. A node stores the
stable tool type ID, its exact semantic version, and a `NodeConfiguration` whose
assignments keep their authored order and values. Binding a node against the registry
reports an unknown tool, an unsupported version, a type-incompatible value, or a missing
required parameter as diagnostics; it never removes or rewrites stored configuration, and
an undeclared parameter key is preserved as a warning.

`SubworkflowDefinition` publishes a reusable graph through an ordinary versioned tool
interface. Exposed ports map to internal node ports and exposed parameters are promoted
to node parameters. Structural violations, including a subworkflow that contains itself,
fail at construction. Mismatches that depend on registry resolution surface as
diagnostics so that one missing plugin cannot mask the rest of the contract.

`WorkflowConnection` and `WorkflowGraph` complete the node contracts. Topology is checked
without a registry: unique node IDs, known endpoints, unique connections, no self edge, and
no cycle. Binding adds the contract-dependent checks: an undeclared port, a data type
mismatch, a second source on a single-source input, and a required input with no source. A
caller may declare inputs that are fed from outside the graph, which is how a subworkflow
satisfies the internal inputs its interface maps.

`register_builtin_tools` enumerates the initial generic catalog: 66 versioned built-in
definitions across Acquisition, Image Processing, Location and Geometry, Measurement,
Recognition and Text, Clustering and Structure, and Logic and Results. The definitions are
contracts only; the vision and recognition milestones add the implementations and refine
the parameter sets. Both reference graphs bind against this one catalog with no
application-specific tool, and a test asserts that no definition carries reference
application vocabulary.
