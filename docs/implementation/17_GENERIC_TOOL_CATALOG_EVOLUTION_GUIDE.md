# 17 — Generic Tool Catalog Evolution Guide

**Product:** NTech NVision

**Document type:** Cross-milestone design and implementation guide

**Status:** Working development guide

**Date:** 2026-08-28

**Related:** `16_GENERIC_VISION_TOOL_PLATFORM_EXECUTION_PLAN.md`, ADR-0005,
ADR-0006, and ADR-0007

---

## 1. Purpose

This guide keeps the NVision Tool Catalog reusable as the platform grows beyond the Wire
Marker and Pin Insert references. It defines how a capability qualifies as a built-in tool,
which current contracts need to evolve before later milestones depend on them, and how new
application evidence should extend the catalog without placing product vocabulary in Core.

The intended claim is deliberately precise:

> NVision is being designed as a general industrial Machine Vision platform. Wire Marker
> and Pin Insert are independent reference oracles used to challenge that design; they do
> not define the complete platform or prove every future application family.

The initial 66 definitions are the first catalog needed by those references. They are not a
closed or universally complete Machine Vision catalog.

---

## 2. Generalization evidence model

Generality is demonstrated in increasing levels of evidence:

1. **Product-neutral vocabulary** — no built-in ID, port, result, ABI, or SDK contract names
   Wire Marker, Pin Insert, or another customer product.
2. **Independent semantics** — a tool can be explained, configured, and tested without a
   reference application's business rules.
3. **Composable execution** — its typed inputs and outputs can actually flow through the
   runtime; a graph that validates must not depend on values the execution model cannot
   represent.
4. **Second-use evidence** — the same contract is exercised by a materially different
   workflow without widening it with product flags.
5. **Operational evidence** — ownership, cancellation, diagnostics, evidence, serialization,
   UI presentation, and performance remain correct outside a unit-test harness.

Wire Marker currently supplies recognition, multi-image, scoring, lookup, decision, and
evidence pressure. Pin Insert supplies segmentation, location, alignment, clustering,
point-grid, learning, and geometric-measurement pressure. Before describing the catalog as
broadly proven, contract exercises should also cover at least three different application
families, selected when real requirements or datasets are available:

- surface-defect or segmentation inspection;
- calibrated dimensional measurement;
- multi-camera traceability combining barcode, OCR, and presence/absence;
- generic AI classification or object detection;
- temporal or multi-frame inspection.

These exercises need not become full products. Their graphs must bind and execute through
the ordinary catalog and expose any missing generic capability explicitly.

---

## 3. Capability placement

Use the smallest level that preserves understandable and reusable semantics.

### Level A — Atomic built-in tool

Use for stable operations with independent typed inputs and outputs, such as threshold,
morphology, connected components, line fit, distance, text recognition, or compare.

### Level B — Reusable strategy or subworkflow

Use for replaceable pipelines assembled from atomic behavior, such as multi-orientation
OCR, feature location, print-quality analysis, text consensus, or point-grid inspection.

### Level C — Project, Job, Workflow, or Recipe configuration

Use for product rules, expected text, model selection, capture roles, physical meaning,
thresholds, tolerances, lookup tables, and final decision policy.

### Level D — Isolated application plugin

Use only when configuration and reusable tools cannot express a necessary rule safely.
The plugin remains outside Core and consumes and produces generic contracts. Promotion to
Level A or B requires a second independent use case with stable product-neutral semantics.

Algorithm availability alone does not justify a Toolbox entry. A tool must remain
understandable to a vision engineer and composable through Studio and Runtime.

---

## 4. Tool admission checklist

A new or revised catalog definition is admitted only when all applicable questions have a
concrete answer.

### Identity and semantics

- Is the stable ID independent of category path, backend, customer, and product?
- Can its behavior be described without reference-application vocabulary?
- Is it atomic behavior, a reusable strategy, configuration, or an application plugin?
- Would a backend change preserve the same semantics, or require a new version/tool?

### Typed data and collections

- Does every port have a native representation and an executable transport path?
- Are zero, one, and many values represented explicitly rather than inferred from a plural
  port name?
- Are ordering, duplicate handling, fan-in, fan-out, empty collections, and partial results
  defined?
- Does the output deserve a domain-neutral result type instead of string-keyed
  `MeasurementList` or `any_metadata`?

### Geometry, units, and raster provenance

- Does every spatial value name its coordinate space?
- Does every length or area retain its unit and coordinate/calibration context?
- Do crop, resize, affine, perspective, calibration, and alignment operations publish the
  transforms downstream nodes need?
- Is the pixel-center versus pixel-corner convention declared and tested?
- Can a computed raster be distinguished from a captured frame throughout evidence and
  result handling?

### Execution and failure semantics

- Are configuration errors separate from recoverable frame/vision failures and technical
  faults?
- Are determinism and concurrency characteristics truthful?
- Does claimed cancellation or timeout support stop or bound work, rather than report only
  after the work has completed?
- Are resource/model sessions owned at the declared lifetime and safe under concurrency?

### Authoring and operation

- Can Studio render and edit the schema without a tool-specific screen?
- Can Runtime display the result and evidence without knowing the application?
- Can missing plugins, resources, models, and versions remain unresolved without data loss?
- Can C ABI, gRPC, and persisted schemas represent the contract without exposing a
  third-party type?

### Verification

- Do tests include non-identity and failure cases, not only identity/happy paths?
- Is the algorithm checked against an exact property, a golden result, or a reference
  baseline appropriate to its risk?
- Are ownership, coordinate round-trip, determinism, cancellation, and serialization
  tested where applicable?
- Has at least one real workflow exercised the definition end to end?

---

## 5. Current catalog reuse assessment

| Group | Reuse assessment | Required evolution |
|---|---|---|
| Acquisition | Generic intent; execution contract incomplete | Typed image collections, capture-role metadata, buffer ordering/capacity/drop policy, and multi-camera ownership |
| Image Processing | High reuse | Separate rectangular view from masked ROI; complete conversion coverage only when required; publish exact transforms |
| Location and Geometry | High reuse | Richer assignment results, non-identity alignment tests, and domain-aware homography validation |
| Measurement | High reuse | Typed histogram/statistical results, calibrated-world units, ROI support, and no unqualified distance floats |
| Recognition and Text | High reuse | Executable text/image collections, barcode lists, model capability metadata, and atomic-versus-subworkflow boundaries |
| Clustering and Structure | Medium to high reuse | Typed cluster membership/noise results, consistent labels/centres, and a complete `PointGridResult` |
| Logic and Results | Generic intent; highest contract risk | Executable fan-in, explicit branch semantics, unit-aware comparison, typed result construction, and removal of primary `any_metadata` paths |
| Overlay and Evidence | High reuse in principle | Encoded or self-describing media, dimensions/pixel format/MIME, storage reference, provenance, and transform metadata |

### 5.1 Strong atomic built-ins

The following families should remain built-in atomic capabilities when their contracts stay
precise:

- colour conversion, channel extraction, rectangular image view, resize, normalise,
  CLAHE, blur, sharpen, threshold, adaptive threshold, colour range, morphology, distance
  transform, affine and perspective warp;
- connected components, contour extraction, blob analysis, enclosing circle, line fit,
  feature detect/describe/match, homography estimate, ECC refinement, point transform, and
  point assignment;
- pixel/object count, distance, radius/diameter, histogram, sharpness, contrast, edge
  density, NCC, and SSIM;
- text detection, text recognition, barcode reading, text normalisation/splitting,
  string similarity, and regex validation;
- K-Means, DBSCAN, Mean Shift, lookup, comparison, Boolean logic, and decision.

Their presence in the catalog does not mean every backend, colour format, detector, or
measurement mode must be added in advance. Extend a definition only from demonstrated
requirements, and version a published contract when its meaning changes.

### 5.2 Reusable strategies rather than indivisible primitives

The following are usually more maintainable as Level B strategies or subworkflows, while
remaining visible in Studio when useful:

- OCR as detection, crop/rectification, recognition, decoding, and confidence policy;
- multi-orientation OCR;
- feature locator with selectable backend;
- text consensus across orientations, frames, cameras, or models;
- print-quality analysis;
- point-grid learn and point-grid inspect.

The atomic stages must remain accessible so another application can replace or debug one
stage without forking the strategy.

---

## 6. Cross-milestone contract corrections

The following are development gates, not optional future polish.

### 6.1 Collections and multi-value ports

`allows_multiple` at graph validation must have an equivalent runtime representation.
`PortValueMap` cannot silently overwrite a second value for the same port. Define one
consistent model before M4 Text Consensus or M5 logic execution depends on it:

- a typed collection value;
- an ordered multi-entry port binding; or
- an explicit collection-building tool and collection port type.

The model must cover capture sequences, recognition crops, barcode results, expression
inputs, weighted measurements, Boolean operands, result evidence, and other fan-in cases.

### 6.2 Coordinate and transform chain

- Affine, perspective, resize, crop, calibration, and alignment outputs must expose enough
  information to map results back to their source.
- Define the pixel coordinate convention and test resize mappings with non-integer scale.
- Verify ECC transform direction against the backend contract using non-identity
  translation, rotation, and perspective fixtures.
- Homography validation must evaluate the projective mapping over the actual image or ROI
  domain, including corners, homogeneous denominator, orientation, convexity, area ratio,
  and local scale where applicable.

### 6.3 Typed aggregate results

Do not use `MeasurementList` as a universal container. Introduce product-neutral result
types when structure matters, including:

- `HistogramResult`;
- `ClusterResult` with membership and noise;
- `PointAssignmentResult`;
- `BarcodeResultList`;
- `CapturedImageSet` or equivalent;
- `PointGridResult` with matched, missing, extra, and deviation data;
- a typed inspection-result builder input/output.

Avoid `any_metadata` as the primary path through a typed graph. Controlled extensibility
may remain at plugin or metadata boundaries, but ordinary built-ins need contracts that
Studio, Runtime, C ABI, gRPC, and persisted schemas can inspect.

### 6.4 Evidence media

An evidence consumer must be able to read an artifact without out-of-band knowledge. Image
and overlay evidence must be encoded or accompanied by at least:

- width and height;
- pixel format/channel order;
- encoding or MIME type;
- content integrity and length;
- storage reference;
- coordinate space and relevant transform/provenance.

### 6.5 Truthful execution characteristics

Tool metadata must not claim cancellation, timeout, thread safety, or reentrancy that the
implementation does not provide. Long-running OpenCV and inference operations need
cooperative checkpoints or scheduler isolation appropriate to their backend. Measuring an
elapsed budget after completion is diagnostic behavior, not preemptive timeout.

### 6.6 Subworkflow assignability

Parameter promotion compatibility must compare the complete accepted value domain, not
only `ParameterKind` and the exposed default. Numeric ranges, enumeration options, array
element/count constraints, geometry kinds, ROI shapes, and resource kinds must be
assignable from the exposed schema to the internal schema.

### 6.7 Immutable raster enforcement

OpenCV interoperability currently borrows immutable NVision bytes through a mutable
`cv::Mat` API. Tool implementations must never mutate borrowed input. Keep cloning for
potentially in-place operations and add regression checks that source bytes remain
unchanged after representative and high-risk tools.

---

## 7. Milestone entry and exit gates

### M4 — Model and recognition

Before completing M4:

- choose and implement the collection/multi-value model required by Text Consensus,
  recognition crops, and barcode lists;
- define `TextResult`, text collections, barcode collections, detection-to-crop mapping,
  and orientation result semantics;
- keep OCR as a composable strategy over independently testable stages;
- verify model package provenance before importing external assets;
- golden-test preprocessing, tensor layout, decoding, orientation, and post-processing.

### M5 — Workflow, Job, and Recipe execution

Before completing M5:

- execute fan-in and fan-out without overwriting values;
- define data-selection versus control-flow branch semantics;
- resolve promoted parameters using full schema assignability;
- bind resource/model/template collections deterministically;
- define typed result aggregation and point-grid inspection results;
- make cancellation, timeout, scheduling, lifetime, and failure propagation observable and
  truthful.

### M8 — C ABI, SDK, and Engine Host

Before completing M8:

- serialize every collection and typed aggregate without C++ or third-party ABI leakage;
- preserve ownership and cancellation across C ABI/P/Invoke;
- expose self-describing evidence and immutable result queries;
- prove local and gRPC surfaces express the same generic contract.

### M9/M10 — Studio and Runtime

Before completing the UI milestones:

- render collection ports, typed aggregates, unresolved nodes, and promoted parameters
  without application-specific screens;
- visualize coordinate spaces and transform provenance during authoring/debug;
- keep Runtime evidence and overlay presentation read-only;
- use real native execution through the supported boundary, never UI-only mock results.

---

## 8. Candidate catalog expansion

The following capabilities are common across broader industrial applications and should be
considered when a real workflow requires them:

- mask Boolean operations and Apply Mask;
- geometry filter/select/convert and transform compose/invert;
- lens undistortion, camera calibration, and pixel-to-world calibration;
- template matching and edge/line/circle location strategies;
- unit-aware tolerance evaluation and measurement reducers;
- generic AI classification, object detection, segmentation, and NMS/post-processing;
- collection construction, map/reduce, fan-in/fan-out, and bounded aggregation;
- temporal tracking and multi-frame consensus;
- defect list construction, filtering, scoring, and evidence selection.

Do not pre-create empty tools for this list. Admission requires a concrete consumer, typed
semantics, an implementation boundary, and proportionate verification.

---

## 9. Verification policy

Catalog coverage counts only when a definition is both registered and executable through
its declared contract. A green unit suite is necessary but not sufficient.

For each relevant tool or strategy, verification should combine:

1. contract validation and negative cases;
2. exact algorithmic properties or golden fixtures;
3. non-identity coordinate and transform cases;
4. deterministic repeat execution;
5. ownership and source-byte immutability;
6. missing/corrupt resource and backend failures;
7. cancellation, timeout, concurrency, and memory behavior;
8. end-to-end execution in at least one real workflow.

Milestone documentation must distinguish implemented coverage, reference-graph coverage,
and production-proven coverage rather than treating them as the same claim.
