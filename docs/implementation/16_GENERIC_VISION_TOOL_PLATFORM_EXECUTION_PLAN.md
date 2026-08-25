# 16 — Generic Vision Tool Platform and Reference Applications Execution Plan

**Product:** NTech NVision
**Document type:** Long-running implementation plan and cross-session context
**Status:** Approved direction; implementation not started
**Date:** 2026-08-25
**Primary deliverable:** A real NVision Studio/Runtime application that executes Wire Marker Inspection end to end through the generic native tool platform
**Second reference application:** Pin Insert Machine, used to prevent Wire Marker-specific abstractions from entering Core

---

## 1. Purpose

This document is the authoritative execution context for the next implementation sessions.
The work continues milestone by milestone until a real application runs reliably through
the complete NVision path. It must not stop at architecture diagrams, isolated libraries,
mock UI, or a Wire Marker-specific API.

The platform goal is:

> Build the most general practical Machine Vision tool platform possible, then express
> concrete applications as Projects, Jobs, Workflows, and Recipes assembled from those
> tools.

Wire Marker Inspection is the first complete application and acceptance vertical slice.
It is not a built-in Core tool. Pin Insert Machine is a second, materially different
reference application used to validate that the abstractions are genuinely reusable.

---

## 2. Mandatory architectural correction

The following model is incorrect:

```text
Native Core
└── Wire Marker Inspection Tool
```

The required model is:

```text
Generic NVision Platform
├── Native Core contracts
├── Generic Tool Catalog
├── Generic Workflow Engine
├── Generic Job and Recipe execution
├── Native OpenCV / ONNX Runtime tool implementations
├── C ABI / PInvoke
├── gRPC Engine Host
├── NVision Studio
└── NVision Runtime
        │
        ├── loads Wire Marker Project / deployment package
        └── can later load Pin Insert Project / deployment package
```

No production Core, C ABI, gRPC, SDK, Studio, or Runtime type may contain Wire Marker or
Pin Insert vocabulary unless it lives in a reference Project or an explicitly isolated
application plugin.

Forbidden examples:

```text
nv_wire_marker_inspect_pair
WireMarkerInspectionEngine
PinInsertInspectionEngine
WireMarkerType
PinInsertResult in generic SDK contracts
```

Required generic examples:

```text
ToolTypeDefinition
WorkflowDefinition
Job
Recipe
ExecutionSession
InspectionResult
TextResult
FeatureMatchResult
PointGridModel
DefectList
Decision
```

---

## 3. Non-negotiable platform boundaries

1. Canonical Core, Workflow execution, image processing, OpenCV, and ONNX Runtime hot
   paths are portable C++20.
2. Studio, Runtime UI, Runtime supervision, persistence, and business-oriented services
   use C#/.NET 10.
3. Local Studio test/debug uses the versioned C ABI through P/Invoke.
4. Production Runtime supervises the same native engine through the gRPC Engine Host.
5. Runtime does not depend on Studio. Production continues when Studio or Runtime UI is
   closed.
6. Full-resolution image buffers remain native. Managed clients receive handles,
   bounded previews, metadata, and immutable results.
7. Tool APIs are coarse-grained and never expose `cv::Mat`, ONNX Runtime types, vendor
   SDK objects, or the C++ ABI.
8. Technical execution status is separate from the application OK/NG decision.
9. Tool category paths are presentation metadata, not identity.
10. Application-specific rules belong in Project/Workflow/Recipe configuration. When
    configuration is insufficient, use an isolated project plugin before considering a
    new built-in Core tool.

---

## 4. Current repository baseline

The repository already contains:

- managed P0–P2 behavior as a compatibility oracle;
- N0 native C++ foundation and versioned C ABI;
- N1 native immutable Frame/FrameBuffer ownership, File Camera, acquisition lifecycle,
  cancellation, C ABI, and .NET `SafeHandle` facade;
- a native Engine Host protobuf scaffold;
- managed Workflow, Job, Recipe, model package, ONNX Runtime, and Wire Marker vertical
  slice behavior that must not be expanded as the production hot path.

N1 implementation commit:

```text
ed53d73 feat(native): move acquisition ownership to C++
```

It was merged into `develop` by:

```text
bd46168 merge: integrate native acquisition and vision platform plan
```

---

## 5. Reference application A — Wire Marker Inspection

### 5.1 Source of truth

```text
D:\src\learn_opencv_all\WireMarkerInspection
```

Dependencies and assets:

```text
D:\src\learn_opencv_all\Libraries
D:\src\learn_opencv_all\output\Release\wire_marker
D:\src\learn_opencv_all\output\Release\model
```

The current executable baseline has been verified as:

```text
SELF_TEST PASS - 0 failure(s)
BATCH_ACCURACY=13/13
```

The project uses PaddleOCR-origin detector and recognizer models converted to ONNX and
executed by ONNX Runtime CPU. It does not currently execute through Paddle Inference.

### 5.2 Application behavior

Each product is represented by two captures:

```text
TYPE{1..4}-I{1|2}-{product-index}
```

The application:

1. acquires or loads I1 and I2;
2. detects the marker text region;
3. recognizes text at 0 and 180 degrees;
4. normalizes and reconciles the two OCR results;
5. maps the two orientation results to TYPE 1–4;
6. performs print-quality/OCV analysis;
7. validates expected text or pattern;
8. combines results into a final decision;
9. exposes overlays, metrics, timing, and evidence.

### 5.3 General capabilities extracted from Wire Marker

- multi-image acquisition and capture-role grouping;
- text detection;
- perspective crop/rectification;
- text recognition;
- configurable multi-orientation OCR;
- text normalization, split, similarity, matching, and consensus;
- regex/pattern validation;
- morphology and connected-components analysis;
- sharpness, contrast, histogram, continuity, and print-quality measurements;
- defect region extraction;
- weighted scoring, lookup tables, comparisons, Boolean logic, and decisions;
- evidence and overlay generation.

### 5.4 Wire Marker-specific behavior

The following must remain in the Wire Marker Project, Recipes, or an isolated project
plugin:

- I1/I2 product semantics;
- filename convention used by the regression dataset;
- orientation-to-TYPE mapping;
- slash-separated two-part marker semantics;
- punctuation recovery and current pair-reconciliation heuristics;
- expected texts and order sensitivity for TYPE 1–4;
- tuned OCR acceptance constants;
- tuned OCV weights and thresholds;
- application decision and evidence policy.

---

## 6. Reference application B — Pin Insert Machine

### 6.1 Source of truth

```text
D:\entry\NCore\NProjects\PinInsertMachine
```

Only acquisition, algorithm, recipe, result, and execution behavior is relevant. The
legacy WinForms UI, MFC plumbing, logging utilities, and vendor-specific wrappers are
references, not migration targets.

### 6.2 Observed production algorithm flow

The current native application contains:

- Hikrobot and Basler camera integration;
- raw BayerGR-to-BGR conversion;
- template/golden ROI learning;
- HSV segmentation;
- morphology;
- distance transform and connected-components seed extraction;
- contour growth and minimum-enclosing-circle pin detection;
- K-Means, DBSCAN, or Mean Shift row clustering;
- line fitting, row ordering, and learned grid tolerance;
- AKAZE, ORB, and SIFT feature matching;
- RANSAC homography estimation;
- perspective warp and aligned crop;
- optional ECC refinement;
- NCC, SSIM, and edge-density verification;
- expected-grid versus detected-point association;
- missing-pin and extra-pin detection;
- OK/NG result and evidence image generation;
- recipe tuning and golden-template lifecycle.

The application also links or contains integration code for TBB, ZXing, libdmtx,
camera vendor SDKs, light controllers, and other legacy infrastructure. Those
dependencies must not be copied wholesale into Core.

### 6.3 General capabilities extracted from Pin Insert

- industrial camera source and camera plugin abstraction;
- Bayer/color conversion;
- ROI/image view;
- HSV/color-range segmentation;
- blur, sharpen, CLAHE, normalization, threshold, and morphology;
- distance transform;
- connected components and contour extraction;
- blob/circle center and radius measurement;
- feature detection and matching with selectable backend;
- homography estimation and validation;
- perspective transform and aligned image extraction;
- ECC alignment refinement;
- NCC, SSIM, edge-density, and confidence fusion;
- K-Means, DBSCAN, and Mean Shift clustering;
- line fitting, sorting, and point-grid learning;
- point-grid matching with missing/extra feature results;
- learn/train resource lifecycle;
- template, grid-model, and calibration-like resource binding;
- inspection overlay and evidence generation.

### 6.4 Pin Insert-specific behavior

The following belongs to the Pin Insert Project, Recipes, device configuration, or an
isolated application plugin:

- the physical meaning of a pin;
- the expected connector/product grid;
- product-specific HSV ranges;
- product-specific template ROI;
- selected clustering strategy and tuned parameters;
- acceptable grid tolerance and missing/extra policy;
- camera count and camera-to-operation mapping;
- golden-learning operator workflow;
- product naming and image-save policy;
- machine-specific light/camera/device configuration;
- final business decision and PLC behavior.

### 6.5 Current data gap

The initial inventory did not find golden/regression image assets under the provided
`PinInsertMachine` project directory. Before Pin Insert parity can become a quality gate,
the implementation session must locate the actual production/simulation images and
expected results or request them after exhausting the known project/configuration paths.
Until then, Pin Insert is still valid as an architectural and algorithmic reference, but
not as a reproducible parity suite.

---

## 7. Generalization model

Every candidate capability is classified into one of four levels.

### Level A — Generic built-in atomic tool

Use when the operation has stable, application-independent semantics and useful typed
inputs/outputs.

Examples:

```text
Color Convert
Crop / Image View
Threshold / InRange
Morphology
Connected Components
Feature Match
Homography Estimate
Perspective Warp
OCR
Regex Validate
NCC / SSIM
Lookup Table
Compare
Decision
```

### Level B — Generic reusable subworkflow or strategy

Use when several atomic operations form a reusable pipeline but users may still need to
inspect or replace parts.

Examples:

```text
Multi-Orientation OCR
Feature Locator with backend strategy
Print Quality Analysis
Point Grid Learn
Point Grid Inspect
Text Consensus
```

These may appear in the project toolbox as reusable subworkflows without becoming
special-purpose Core engines.

### Level C — Project Workflow/Recipe configuration

Use for product rules, thresholds, model/resource selection, capture roles, expected
layout, and final decision policy.

Examples:

```text
Wire Marker orientation-to-TYPE lookup
Wire Marker expected text profiles
Pin Insert expected golden grid
Pin Insert HSV thresholds
OK/NG policy
```

### Level D — Isolated application plugin

Use only when a required rule cannot reasonably be represented through generic tools or
configuration. It must remain outside Core and use generic input/output contracts.

A Level D capability may be promoted to Level A or B only after a second independent use
case demonstrates stable, product-neutral semantics.

---

## 8. Initial generic Tool Catalog

This is the first implementation catalog needed to cover both reference applications.
It is not the final NVision catalog.

### 8.1 Acquisition

```text
Image Source
File Camera
Capture Sequence / Multi-Image Acquisition
Image Buffer
Image Output
```

Industrial camera and light implementations are plugins behind generic contracts.

### 8.2 Image Processing

```text
Color Convert
Channel Extract
Crop / Image View
Resize
Normalize
CLAHE
Gaussian Blur
Sharpen
Threshold
Adaptive Threshold
Color Range / InRange
Morphology
Distance Transform
Perspective Warp
Affine Transform
```

### 8.3 Location and Geometry

```text
Connected Components
Contour Extraction
Blob Analysis
Minimum Enclosing Circle
Line Fit
Feature Detect and Match
Homography Estimate
Homography Validate
ECC Refine
Point Transform
Point Assignment
```

### 8.4 Measurement

```text
Pixel Count
Object Count
Distance
Radius / Diameter
Histogram
Sharpness
Contrast
Edge Density
NCC
SSIM
Grid Deviation
```

### 8.5 Recognition and Text

```text
Text Detection
Text Recognition
OCR
Barcode Reader
Text Normalize
Text Split
String Similarity
Regex Validate
Text Consensus
```

### 8.6 Clustering and Structure

```text
K-Means
DBSCAN
Mean Shift
Sort Points Along Line
Point Grid Learn
Point Grid Inspect
```

Clustering tools should be exposed only when their typed contracts and execution
semantics remain understandable in Studio. Internal algorithms do not automatically
deserve a Toolbox entry.

### 8.7 Logic and Results

```text
Lookup Table
Expression
Weighted Score
Compare
Boolean Logic
Branch
Decision
Result Builder
Evidence Output
```

---

## 9. Tool contract requirements

A reusable `ToolTypeDefinition` must describe:

```text
Identity
├── stable ToolTypeId
├── semantic version
└── built-in or plugin origin

Catalog metadata
├── display name
├── category path
├── tags/search aliases
├── description/help
└── optional icon/editor hints

Contract
├── typed input ports
├── typed output ports
├── parameter schema
├── resource/model/calibration references
└── result/evidence schema

Execution
├── lifetime
├── thread safety
├── reentrancy
├── determinism
├── CPU/GPU/device requirements
├── cancellation/timeout
└── debug preview support
```

Tool category can change without invalidating a Workflow. A Workflow persists stable
type ID and version, never the display-tree position as identity.

Missing plugins produce unresolved nodes and diagnostics. Studio must not delete or
silently rewrite their configuration.

---

## 10. Typed data and ownership requirements

The existing managed `PortDataType` is a useful oracle but is not sufficient for the
native production platform. Required native types include:

```text
Frame / Image
Image View
Mask
ROI / Region
CoordinateSpaceId
Point2D / PointList
Line2D
Circle
Contour
BoundingBox / Polygon
Transform / Homography
FeatureSet / MatchSet
Detection / DetectionList
TextResult
BarcodeResult
Measurement / MeasurementList
Defect / DefectList
PointGridModel / PointGridResult
Boolean / Integer / Float / String
Decision
Evidence / Overlay
AnyMetadata with controlled compatibility
```

Geometry always carries or references its coordinate space. Cropping, alignment, and
warping must produce explicit transforms. A point or ROI without coordinate-space
meaning is not safe for a general vision workflow.

Images move through native leases/views. Tools must not copy full frames between nodes
unless an operation explicitly materializes a new image.

---

## 11. Generic application execution model

```text
ToolTypeDefinition
        │ instantiate
        ▼
WorkflowNode ── typed connections ── WorkflowDefinition
                                             │
                                             ▼
                                            Job
                                             ▲
                                             │ overrides/bindings
                                           Recipe
                                             │
                                             ▼
                                      Inspection Run
```

- Tool Definition describes reusable capability.
- Workflow Node is a configured instance.
- Workflow is the typed graph.
- Job is one reusable inspection operation.
- Recipe configures a Job for a product/SKU/variant.
- Inspection Run executes one immutable Job/Recipe/resource set.

Technical execution status and inspection verdict are separate:

```text
Technical status: Success / Error / Timeout / Cancelled
Inspection verdict: OK / NG / Indeterminate
```

---

## 12. Reference workflows

### 12.1 Wire Marker authoring graph

```text
Capture Sequence [roles: I1, I2]
    ├── I1 → Text Detection → Text Recognition [0,180]
    └── I2 → Text Detection → Text Recognition [0,180]

OCR results
    → Text Normalize
    → Text Split
    → String Similarity / Pointwise Assignment
    → Text Consensus

Orientation I1 + Orientation I2
    → Lookup Table supplied by Recipe
    → Detected product type

Text crops
    → Print Quality measurements
    → Defect extraction
    → Weighted Score
    → Compare with Recipe thresholds

Recognized text
    → Regex / expected pattern validation

Type + quality + pattern outcomes
    → Boolean Logic
    → Decision
    → Result Builder
    → Evidence Output
```

The project may package this graph as a reusable project subworkflow. It must not register
`Wire Marker Inspection` as a built-in Core tool.

### 12.2 Pin Insert training graph

```text
Golden Image
    → Color Convert
    → ROI
    → HSV InRange
    → Morphology
    → Distance Transform
    → Connected Components
    → Contour / Circle Centers
    → K-Means | DBSCAN | Mean Shift
    → Fit and sort rows
    → Point Grid Learn
    → versioned PointGridModel resource
```

### 12.3 Pin Insert inspection graph

```text
Camera Frame
    → Bayer to BGR
    → Feature Match [AKAZE | ORB | SIFT]
    → Homography Validate
    → optional ECC Refine
    → Perspective Warp
    → NCC / SSIM / Edge verification
    → HSV InRange
    → Morphology
    → Distance Transform
    → Connected Components
    → Pin center detection
    → Point Grid Inspect [Recipe-selected grid model]
    → Missing/extra feature results
    → Decision
    → Result Builder
    → Evidence Output
```

The platform sees points, circles, grids, measurements, defects, and decisions. The Pin
Insert Project gives those results their application meaning.

---

## 13. Target repository ownership

The exact directory names may be refined during implementation, but ownership must remain:

```text
native/
├── core/             generic IDs, status, result, geometry, resources
├── acquisition/      frame/buffer/camera lifecycle
├── workflow/         registry, validation, compile, scheduling, execution
├── vision/           generic OpenCV tools
├── recognition/      OCR, barcode, text contracts
├── ai/               model packages and ONNX Runtime backends
├── calibration/      transforms and calibration resources
├── plugins/          native plugin contracts/loader
├── abi/              versioned generic C ABI
└── host/             generic gRPC Engine Host

sdk/dotnet/           generic managed facade and SafeHandle ownership
apps/studio/          generic .NET 10 authoring/test UI
apps/runtime/         generic .NET 10 supervisor/operator UI

reference-projects/
├── wire-marker-inspection/
│   ├── jobs/
│   ├── workflows/
│   ├── recipes/
│   ├── models/
│   ├── resources/
│   └── tests/
└── pin-insert-machine/
    ├── jobs/
    ├── workflows/
    ├── recipes/
    ├── templates/
    ├── resources/
    └── tests/
```

Do not create one CMake library or .NET project per tool. Group implementations by
capability and isolate only dependency/plugin boundaries.

---

## 14. End-to-end runtime topology

```text
NVision Studio
    └── generic Engine Client
        ├── C ABI / PInvoke for local authoring, test, and debug
        └── Runtime API for publish and diagnostics

NVision Runtime UI
    └── Runtime Supervisor
        └── gRPC
            └── Native Engine Host
                └── Generic native Engine
                    ├── Tool Catalog
                    ├── Workflow
                    ├── OpenCV tools
                    ├── ONNX Runtime tools
                    └── Core
```

Studio and Runtime use the same native engine implementation. Runtime never routes
production execution through Studio.

---

## 14.1 Studio/Runtime image canvas and drawing-control reference

When M9 or M10 reaches image presentation, ROI authoring, geometry editing, or result
overlay work, inspect the existing Geo Measure implementation before creating new UI
controls. It already solves many interaction and visual-lifecycle problems that also
exist in a machine-vision canvas.

Reference repository:

```text
D:\src\ai-workspace\06_Repositories\sol-drcp\robo-station
```

Primary reference files:

```text
docs/design/GEO_MEASURE_DESIGN.md
src/RoboStation.Domain/GeoMeasure/GeoMeasureModels.cs
src/RoboStation.Domain/GeoMeasure/GeoDrawingSession.cs
src/RoboStation.Domain/GeoMeasure/GeoMeasurementCalculator.cs
src/RoboStation.Desktop/ViewModels/GeoMeasure/GeoMeasureViewModel.cs
src/RoboStation.Desktop/Views/GeoMeasure/GeoMeasureView.xaml
src/RoboStation.Desktop/Views/GeoMeasure/GeoMeasureView.xaml.cs
tests/RoboStation.Domain.Tests/GeoMeasurementCalculatorTests.cs
tests/RoboStation.Desktop.Tests/GeoMeasureViewModelTests.cs
```

Reuse or adapt these proven design ideas:

- a pure drawing-session state machine for draft points, candidate geometry, commit,
  finish, undo-point, cancel, and validity;
- explicit drawing modes for point/marker, line/polyline, polygon, rectangle, and
  circle, with room for later tool-defined ROI editors;
- ViewModel ownership of tool mode, feature collection, selection, commands, and
  user-visible state;
- View/code-behind ownership of pointer capture, screen/canvas coordinate conversion,
  viewport projection, and visual-element lifecycle;
- separate full-bleed drawing/overlay layers and an unscaled HUD/control layer;
- selection and hit testing for points, line distance, polygon containment, shapes,
  labels, and resize/edit handles;
- persistent labels and measurement/result callouts;
- zoom in, zoom out, wheel zoom around the pointer, pan, fit image, center, 1:1 view,
  zoom-to-selection, and reset view;
- coalesced redraw on the WPF render queue rather than rebuilding visuals for every
  raw pointer or viewport event;
- complete re-projection after pan, zoom, resize, image replacement, frame change, or
  coordinate-space change;
- keyboard interaction such as Enter/Finish, Backspace/Undo point, Escape/Cancel, and
  temporary pan while drawing;
- selection indication that does not depend on color alone;
- unit tests for drawing transitions and geometry, plus real WPF startup/smoke tests.

Do not copy the geographic domain model into NVision. Geo Measure uses GMap and WGS84,
whereas NVision must use typed image and calibration coordinate spaces defined by the
native contracts. Reuse the interaction patterns and separation of responsibilities,
but replace geographic types, projection, and distance/area calculations with:

- image-pixel, ROI-local, normalized-image, model-input, calibrated-world, and display
  coordinate spaces;
- explicit transforms with source/target space identity and frame/image identity;
- native-compatible generic geometry contracts for point, line/polyline, polygon,
  axis-aligned rectangle, rotated rectangle, circle, ellipse, and tool overlays;
- image measurements and optional calibrated measurements implemented by generic
  native tools, not inferred from display pixels;
- immutable engine results/evidence separated from editable Studio annotations.

The preferred reusable UI boundary is a generic Studio controls assembly, for example
`NTech.NVision.Studio.Controls`, rather than controls embedded in Wire Marker or another
application. Its first vertical slice should contain:

```text
ImageViewerCanvas
  ViewportController          pan, zoom, fit, center, 1:1, coordinate conversion
  ImageLayer                  bounded managed presentation of the current native frame
  DrawingLayer                editable ROI and geometry drafts
  ResultOverlayLayer          immutable engine overlays and evidence
  AnnotationLayer             labels, measurements, handles, selection, hover
  InteractionController       pointer capture, gestures, keyboard, hit testing
  DrawingSession              draft/commit/undo/cancel state independent of WPF
  OverlayRendererRegistry     renderer selected by generic overlay/geometry kind
```

The canvas must initially support:

- image display, loading state, no-image state, and pixel inspection;
- point, line/polyline, polygon, axis-aligned rectangle, rotated rectangle, circle,
  and ellipse rendering; rectangle, polygon, line, and circle are required editors;
- create, select, move, resize, rotate where applicable, delete, cancel, undo, redo,
  finish, clear, and zoom-to geometry;
- configurable stroke/fill/label styles, layer visibility, ordering, and overlay
  filtering without application-specific visual types;
- clipping to image bounds where required and deliberate support for out-of-image
  geometry where a tool contract permits it;
- stable rendering across DPI scaling, window resize, image resolution changes, rapid
  live-preview frames, and viewport operations;
- a read-only presentation mode for Runtime and an authoring mode for Studio.

Studio uses authoring mode for ROI, fixture, mask, measurement, search-region, and
training-resource editing. Runtime reuses the viewer and overlay renderer in read-only
mode for live preview, result review, alarms, and evidence; production inspection must
not depend on the viewer being open or able to keep up with every frame.

Before reusing code directly, verify its license, target-framework compatibility, and
dependency direction. Extract domain-independent behavior with tests; do not create a
source-level dependency from NTech to the `robo-station` application.

---

## 15. Implementation milestones

### M0 — Baseline and branch integration

- merge N1 only after authorization;
- create a dedicated feature branch for the generic tool platform;
- record Wire Marker source, model, dataset, and executable baselines;
- locate Pin Insert datasets, golden templates, configuration, and expected outputs;
- audit licenses for external models and native dependencies;
- establish Windows x64 build first while preserving Linux-native portability.

Exit gate:

- N1 remains green;
- Wire Marker 13/13 baseline is reproducible;
- Pin Insert data availability is documented;
- no external absolute path is introduced into production configuration.

### M1 — Native foundational data contracts

- generic IDs and semantic versions;
- typed port/data contracts;
- coordinate spaces and transforms;
- image views and native ownership;
- immutable execution/result/evidence structures;
- resource/model/template references;
- technical status versus verdict.

Exit gate:

- contracts describe both reference workflows without application-specific types;
- invalid geometry, ownership, and status combinations are rejected by tests.

### M2 — Tool definition, catalog, and parameter schema

- versioned `ToolTypeDefinition`;
- hierarchical category metadata and tags;
- parameter kinds for enum, arrays, geometry, ROI, resource, model, calibration, and
  conditional fields;
- built-in/plugin origin;
- execution characteristics;
- registry and version resolution;
- unresolved node behavior;
- declarative property-editor schema;
- reusable subworkflow contract.

Exit gate:

- a native catalog can enumerate the initial generic tool tree;
- both reference graphs validate at the contract level;
- changing a category does not change persisted identity.

### M3 — Generic OpenCV vision foundation

Implement and test the common tools required by both references:

- color conversion and Bayer decode;
- ROI/image view, resize, crop, affine/perspective warp;
- blur, sharpen, normalize, CLAHE, threshold, color range, morphology;
- distance transform, connected components, contours, blobs, circles, line fitting;
- feature matching, homography, ECC refinement;
- NCC, SSIM, edge density, histogram, sharpness, and contrast;
- clustering and point-grid learn/inspect;
- overlays and evidence primitives.

Exit gate:

- tools are independently golden-tested;
- no result depends on a UI drawing side effect;
- no public contract exposes OpenCV types;
- Pin Insert source algorithms can be represented without a Pin Insert engine class.

### M4 — Generic model and recognition foundation

- `.nvmodel` integrity and manifest validation in C++;
- model registry and session ownership;
- ONNX Runtime native CPU backend;
- text detector preprocessing/postprocessing;
- text recognizer preprocessing/postprocessing;
- CTC decoding;
- configurable orientation search;
- text/string tools and barcode result contracts;
- model/backend capability metadata.

Exit gate:

- invalid package/checksum/tensor contracts fail clearly;
- sessions are instance-owned and concurrency-safe;
- Wire Marker OCR primitives reproduce accepted baseline behavior.

### M5 — Generic native Workflow, Job, and Recipe execution

- workflow validation and compilation;
- typed connection compatibility;
- parameter and Recipe resolution;
- model/resource/template binding;
- scheduler, timeout, cancellation, lifetime, and resource coordination;
- train/learn operation contract separate from inspection execution;
- immutable result aggregation and evidence capture;
- generic subworkflow execution.

Exit gate:

- both reference graphs compile through the same engine;
- the engine contains no Wire Marker or Pin Insert dispatch branch;
- deterministic headless test workflows run successfully.

### M6 — Wire Marker reference project, headless parity

- create the Wire Marker Project/Job/Recipes using generic tools;
- package detector, recognizer, dictionary, profiles, and thresholds as resources;
- express orientation-to-TYPE through generic lookup/configuration;
- express text validation and final decision through generic tools;
- isolate only irreducible reconciliation logic in a project plugin if necessary;
- produce structured results and overlays.

Exit gate:

- all 13 pairs classify correctly;
- OCV GOOD/BAD baseline remains separated;
- batch results are deterministic;
- no `nv_wire_marker_*` API or built-in Wire Marker Core type exists.

### M7 — Pin Insert reference project, generalization audit

- model training and inspection graphs with generic tools;
- bind golden template and point-grid resources through Recipes;
- reproduce HSV segmentation, feature alignment, grid learning, missing/extra matching,
  and decision behavior;
- implement camera/light adapters only behind generic plugin contracts;
- use project plugin fallback only for rules that cannot be generalized safely.

Exit gate:

- once datasets are available, reproduce agreed Pin Insert outcomes;
- document every capability that could not use an existing generic tool;
- reject or redesign any Core contract that embeds assumptions from either reference app.

### M8 — Generic C ABI, .NET SDK, and Engine Host contracts

- catalog enumeration;
- Project/package load and validation;
- Job/Recipe discovery;
- execution session and cancellation;
- immutable result/evidence query;
- progress/trace events;
- opaque handles and explicit ownership;
- .NET `SafeHandle` facade;
- gRPC equivalent for production control and metadata.

Exit gate:

- a generic .NET console client can run either project by IDs/configuration only;
- no application-specific P/Invoke or gRPC method exists;
- native ownership and cancellation tests pass.

### M9 — NVision Studio executable vertical slice

Build a real .NET 10 WPF Studio with:

- dynamic searchable Toolbox tree sourced from Tool Catalog;
- drag/drop Workflow canvas;
- typed ports and interactive connection validation;
- schema-driven Property Editor;
- Project, Job, Workflow, and Recipe editors;
- resource/model/template management;
- generic reusable image canvas/control library, implemented after reviewing the Geo
  Measure references in Section 14.1;
- image viewer with independent image, drawing, result-overlay, and annotation layers;
- rect, rotated-rect, polygon, line/polyline, circle, ellipse, and point rendering, with
  rect, polygon, line, and circle authoring in the first vertical slice;
- selection, handles, hit testing, move/resize/rotate where applicable, undo/redo,
  finish/cancel, delete/clear, and editable tool-defined ROI geometry;
- pan, wheel/pointer-centered zoom, zoom in/out, fit image, center, 1:1,
  zoom-to-selection, coordinate-space conversion, and pixel inspection;
- generic result, measurement, defect, decision, overlay, timing, and trace viewers;
- train/learn action support;
- local P/Invoke execution;
- single-step and batch test/debug;
- save/load/publish-ready authoring artifacts.

Wire Marker must open as data/configuration in Studio. Studio must not contain hardcoded
Wire Marker screens or execution logic.

Exit gate:

- a user can open the Wire Marker Project, inspect its graph, edit a Recipe, select the
  sample folder, run one pair or all pairs, and see OCR, orientation, TYPE, OCV metrics,
  overlays, decision, timing, and errors;
- a user can draw and edit rect, polygon, line, and circle parameters on the actual
  source image without coordinate drift after zoom, pan, resize, DPI, or image changes;
- committed ROI geometry reaches the generic tool parameter contract in the declared
  coordinate space, and saved/reloaded geometry is identical within defined precision;
- the same generic editors can open the Pin Insert graph and its parameters/resources.

### M10 — Generic Runtime and production Engine Host

- supervised native Engine Host;
- Runtime readiness and lifecycle;
- immutable package activation;
- Job/Recipe switching;
- start/stop/trigger execution;
- bounded live preview;
- generic read-only image viewer and overlay rendering shared with Studio controls;
- counters, alarms, last-result, history, and diagnostics;
- image/evidence persistence;
- Engine Host crash detection and safe recovery;
- generic configurable operator dashboard;
- production continues without Studio or Runtime UI.

Exit gate:

- publish Wire Marker from Studio and run it in Runtime through gRPC;
- close Studio and Runtime UI without stopping production execution;
- restart/fault behavior is observable and safe;
- Runtime has no Wire Marker algorithm code.

### M11 — Hardening and release candidate

- native unit/golden tests;
- C ABI ownership/error tests;
- P/Invoke integration tests;
- Engine Host contract tests;
- Studio ViewModel and UI smoke tests;
- corrupt/missing resource tests;
- timeout/cancellation and failure injection;
- concurrency, memory, leak, performance, and soak tests;
- Windows full application build;
- Linux native headless build;
- deployment package checksums and rollback;
- complete operator/developer diagnostics.

Exit gate:

- all Definition of Done requirements in Section 17 are met;
- the managed compatibility path is no longer required for production execution.

---

## 16. Required quality gates

Every milestone must include proportionate tests before proceeding. Final gates include:

```text
Architecture
  No application-specific type or API in generic Core/SDK
  No UI dependency in Engine
  No OpenCV/ONNX/vendor type in public contracts

Correctness
  Wire Marker 13/13
  Wire Marker OCV baseline separation
  Pin Insert parity once source datasets are available
  Deterministic repeat runs

Ownership
  Native frame/image lifetime verified
  No use-after-free across C ABI/PInvoke
  Bounded preview/evidence copies only

Reliability
  Timeout and cancellation
  Missing/corrupt models/resources
  Invalid graph/recipe
  Engine Host crash/restart
  Long-running soak

Performance
  Per-tool and end-to-end timing
  Memory growth bounded
  No per-pixel managed/native calls
  Baseline and target recorded before optimization

Portability
  Windows x64 Studio/Runtime
  Linux native headless build/test
  No production absolute Windows paths

Studio/Runtime image canvas
  Rect, polygon, line, and circle drawing state transitions are unit-tested
  Pan/zoom/fit/1:1 preserve pointer-to-image coordinate correctness
  ROI save/load and managed/native round-trip preserve coordinate space and precision
  Overlays remain aligned after DPI, resize, image/frame, and viewport changes
  Selection, hit testing, edit handles, undo/redo, finish/cancel, and keyboard paths pass
  No ghost draft or stale result overlay survives cancel, image change, or session reset
  Full HD through 4K and Windows scaling from 100% through 200% are smoke-tested
  Rapid preview remains bounded and cannot block production acquisition/execution
  Runtime read-only mode cannot mutate Recipe or engine-owned result geometry
```

Performance thresholds must be measured and then approved; they must not be invented
without a baseline from the reference applications and target hardware.

---

## 17. Definition of Done

The plan is complete only when all of the following are true:

1. NVision Studio is a real executable, not a mock, and can author, validate, save, load,
   test, debug, and publish a generic vision Project.
2. Wire Marker Inspection is represented as Project/Job/Workflow/Recipes, not as a Core
   engine or built-in application tool.
3. The Wire Marker Project runs the real OpenCV and ONNX Runtime algorithms in C++.
4. The 13 reference image pairs pass the agreed result baseline.
5. OCV quality results and overlays are visible and regression-tested.
6. Studio local test uses C ABI/PInvoke and native frame ownership.
7. Runtime loads the published package and executes the same native graph through the
   Engine Host gRPC path.
8. Runtime production is independent of Studio and both UIs.
9. Results, evidence, errors, timing, and traceability are operationally usable.
10. Pin Insert has been used to audit or exercise the same generic contracts, including
    image processing, feature location, geometry, grid learning/inspection, Recipe
    resources, and decisions.
11. No generic API contains Wire Marker or Pin Insert domain vocabulary.
12. Windows full-app and Linux native-headless quality gates pass.
13. Failure, recovery, ownership, performance, and soak gates pass.
14. Documentation, active task, context, decisions, and handoff accurately describe the
    final implementation.

“Application runs perfectly” means the above observable gates are satisfied. It does not
mean subjective absence of all future enhancements.

---

## 18. Cross-session continuation protocol

Every implementation session working on this plan must:

1. read repository `AGENTS.md` and all mandatory documents listed there;
2. read this document completely;
3. inspect the current branch, worktree, latest commits, `tasks/active.md`, and
   `handoff/latest.md`;
4. continue from the first incomplete milestone and its smallest safe vertical slice;
5. avoid re-planning completed work unless tests or new evidence invalidate it;
6. keep commits small and scoped by capability;
7. update tests with each behavior change;
8. update `docs/00_CONTEXT.md`, `tasks/active.md`, `handoff/latest.md`, and this plan's
   progress record before handoff;
9. never merge, publish, delete, or make destructive changes without the authority
   required by repository/user instructions;
10. never stop at a facade/mock when the milestone exit gate requires real native
    execution.

If blocked, document:

```text
Exact blocker
Evidence already gathered
Safe alternatives attempted
Required user/external input
Next executable step after unblock
```

---

## 19. Progress record

Use this table as the durable summary. Detailed work remains in `tasks/active.md` and
`handoff/latest.md`.

| Milestone | Status | Evidence / commit |
|---|---|---|
| N0 — Native foundation | Complete | Existing repository baseline |
| N1 — Native acquisition | Complete and merged | `ed53d73`, merge `bd46168` |
| M0 — Baseline and integration | In progress | Integration/check green; external asset license provenance remains open |
| M1 — Native data contracts | Complete on feature branch | `c07440b` foundations; `0c71340` Image Views; `4339211` resources; `e8b7073` results |
| M2 — Tool definition/catalog | Not started | |
| M3 — Generic OpenCV foundation | Not started | |
| M4 — Model/recognition foundation | Not started | |
| M5 — Native Workflow/Job/Recipe | Not started | |
| M6 — Wire Marker headless parity | Not started | |
| M7 — Pin Insert generalization | Not started | Dataset location still required |
| M8 — C ABI/.NET SDK/Engine Host contracts | Not started | |
| M9 — NVision Studio executable | Not started | |
| M10 — NVision Runtime executable | Not started | |
| M11 — Hardening/release candidate | Not started | |

---

## 20. Immediate next action

M1 is complete on `codex/generic-vision-m1`. Keep the remaining M0 provenance gap explicit
and do not begin M2 without the next instruction:

1. resolve or explicitly defer external model license provenance before importing or
   redistributing assets;
2. begin M2 Tool Definition/Catalog with the smallest generic registry/schema slice;
3. keep every slice generic and prove both reference workflows without application
   vocabulary in production Core types.
