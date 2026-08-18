# NTech NVision Platform Foundation

**Document Type:** Platform Foundation / Product & Technical Baseline  
**Product:** NTech NVision  
**Positioning:** Industrial Vision Intelligence Platform  
**Status:** Foundation Document  
**Version:** 1.0  
**Date:** 2026-08-18  

---

## 1. Purpose

This document defines the product, architecture, terminology, module boundaries, development principles, deployment model, and long-term roadmap for **NTech NVision**.

It is intended to serve as the baseline document for:

- Product planning
- Software architecture
- Repository organization
- UI/UX design
- Machine Vision feature development
- AI integration
- Industrial device integration
- Runtime deployment
- Edge deployment
- SDK and plugin development
- Factory-level management
- Technical documentation
- Future product expansion

The primary objective is to ensure that NVision is developed as a reusable **industrial Machine Vision platform**, rather than as a collection of one-off customer inspection applications.

---

# 2. Product Identity

## 2.1 Company

**NTech**

NTech is the parent product and technology brand.

## 2.2 Platform

# NVision

**Industrial Vision Intelligence**

NVision is NTech's industrial Machine Vision platform for building, deploying, operating, and managing automated visual inspection systems.

Recommended product presentation:

```text
NTECH
NVISION

Industrial Vision Intelligence
```

Alternative compact branding:

```text
NVISION
by NTech
```

---

# 3. Meaning of NVision

The official brand meaning is:

> **NVision = NTech Vision**

Additional meanings may be used in marketing and product communication:

- **N = Next** — Next-generation Machine Vision
- **N = Neural** — AI and Neural Vision
- **Vision** — Acquisition, perception, analysis, inspection, decision, and automation

The platform philosophy is:

```text
SEE
 ↓
UNDERSTAND
 ↓
INSPECT
 ↓
DECIDE
 ↓
ACT
```

NVision must therefore go beyond image processing.

It must connect visual information to industrial decisions and automation.

---

# 4. Product Positioning

## 4.1 Product Definition

> **NVision is an industrial Machine Vision platform for building, deploying, and operating automated visual inspection systems using traditional Computer Vision, AI, and industrial automation.**

Vietnamese positioning:

> **NVision là nền tảng Machine Vision công nghiệp dùng để xây dựng, triển khai và vận hành các hệ thống kiểm tra chất lượng bằng thị giác máy, AI và tự động hóa công nghiệp.**

## 4.2 Core Value

NVision converts:

```text
Industrial Camera Data
        ↓
Computer Vision
        ↓
AI / Measurement / OCR
        ↓
Inspection Logic
        ↓
Industrial Decision
        ↓
PLC / Robot / MES / Operator
```

## 4.3 What NVision Is Not

NVision must not be designed as:

- A single-purpose Wire Marker inspection application
- A single-purpose Gear inspection application
- A simple camera viewer
- A pure AI inference application
- A pure OpenCV wrapper
- A customer-specific hard-coded inspection program
- A project that must be forked for each customer

Customer applications are built **on top of NVision**.

Example:

```text
NVision Platform
│
├── Wire Marker Inspection
├── Gear Inspection
├── Label Inspection
├── PCB Inspection
├── Surface Defect Inspection
├── Assembly Verification
└── Packaging Inspection
```

---

# 5. Product Principles

Every architectural and product decision should follow these principles.

## 5.1 Platform First

Reusable capability belongs in NVision Core.

Customer-specific logic belongs in:

- Project
- Job
- Recipe
- Plugin
- Extension

## 5.2 Configuration Over Hard Coding

Inspection behavior should preferably be expressed through:

- Nodes
- Parameters
- Recipes
- Device configuration
- Models
- Rules

rather than application-specific source code.

## 5.3 Runtime Must Be Deterministic

Production inspection must prioritize:

- Predictable cycle time
- Stable memory usage
- Reliable device communication
- Recoverable failures
- Traceable results

## 5.4 Traditional Vision + AI

NVision must support both:

**Traditional Computer Vision**

- Thresholding
- Edge
- Blob
- Contour
- Morphology
- Geometry
- Measurement
- Pattern matching

and:

**AI Vision**

- Detection
- Classification
- Segmentation
- OCR
- Anomaly detection
- Embedding / similarity

AI is an extension of Machine Vision, not a replacement for every classical vision algorithm.

## 5.5 Hardware Independence

The core architecture should not depend on one camera vendor, PLC vendor, AI framework, or GPU platform.

## 5.6 Edge First

Inspection should be able to run locally without requiring cloud connectivity.

## 5.7 Observability by Design

Every inspection system must provide:

- Logs
- Metrics
- Timing
- Device state
- Alarm history
- Inspection history
- Model version
- Recipe version

## 5.8 Production Safety

Invalid states must never silently produce valid-looking inspection results.

Examples:

- Camera disconnected
- Model unavailable
- PLC communication lost
- Recipe invalid
- Calibration expired
- Inspection node failed

must produce explicit system states.

---

# 6. NVision Product Ecosystem

The long-term product ecosystem is:

```text
                         NTECH
                           │
                        NVISION
                           │
       ┌───────────────────┼────────────────────┐
       │                   │                    │
      BUILD                RUN                MANAGE
       │                   │                    │
NVision Studio       NVision Runtime       NVision Hub
NVision Trainer      NVision Edge
       │                   │
       └─────────┐   ┌─────┘
                 ▼   ▼
              NVision Engine
                    │
       ┌────────────┼─────────────┐
       │            │             │
     Vision         AI         Connect
       │            │             │
       └────────────┼─────────────┘
                    │
               NVision SDK
```

---

# 7. Product Modules

## 7.1 NVision Studio

**Purpose:** Engineering environment for building Machine Vision inspection applications.

Primary users:

- Vision Engineer
- Automation Engineer
- AI Engineer
- Application Engineer
- System Integrator

Main functions:

- Create/Open Project
- Camera configuration
- Lighting configuration
- Image acquisition
- ROI definition
- Vision tools
- AI tools
- Inspection flow
- Measurement tools
- OCR
- Calibration
- Recipe management
- Device mapping
- PLC mapping
- Result configuration
- Debugging
- Simulation
- Offline image testing
- Performance profiling
- Publish deployment package

Recommended tagline:

> **Build Vision. Deploy Intelligence.**

---

## 7.2 NVision Runtime

**Purpose:** Execute published inspection applications in production.

Runtime must support:

- Auto start
- Project loading
- Recipe loading
- Camera acquisition
- Trigger handling
- Vision pipeline execution
- AI inference
- Decision logic
- PLC communication
- Robot communication
- Digital I/O
- Save NG image
- Result logging
- Production counters
- Alarm reporting
- Automatic recovery
- Watchdog
- Health monitoring

Runtime is production software.

Operators should not normally have access to engineering-level algorithm parameters.

---

## 7.3 NVision Engine

**Purpose:** Shared core execution engine used by Studio, Runtime, SDK, and Edge.

Logical architecture:

```text
NVision Engine
│
├── Acquisition Engine
├── Vision Engine
├── AI Engine
├── Workflow Engine
├── Inspection Engine
├── Calibration Engine
├── Device Engine
├── Communication Engine
├── Data Engine
├── Result Engine
├── Logging Engine
└── Plugin Engine
```

The Engine should have minimal dependency on UI technology.

---

## 7.4 NVision AI

**Purpose:** AI inference subsystem for Machine Vision.

Capabilities:

- Classification
- Object Detection
- Instance Segmentation
- Semantic Segmentation
- OCR
- Anomaly Detection
- Visual Similarity
- Embedding
- Feature Extraction

Recommended deployment abstraction:

```text
Training Framework
       │
       ▼
      ONNX
       │
       ├── NVIDIA → TensorRT
       ├── Intel  → OpenVINO
       ├── Windows → DirectML
       └── Generic → ONNX Runtime
```

The project must avoid locking the entire platform to one model framework.

---

## 7.5 NVision Trainer

**Purpose:** Dataset preparation, annotation, training, evaluation, model management, and deployment.

Future capabilities:

```text
Dataset
  ↓
Annotation
  ↓
Train
  ↓
Evaluate
  ↓
Optimize
  ↓
Version
  ↓
Deploy
```

Trainer should eventually manage:

- Datasets
- Classes
- Labels
- Train/validation split
- Augmentation
- Model configuration
- Training jobs
- Metrics
- Model version
- Export format
- Deployment target

Trainer is not required for the earliest NVision MVP.

---

## 7.6 NVision Edge

**Purpose:** Compact deployment for industrial PCs and embedded AI computers.

Target platforms:

- NVIDIA Jetson
- Industrial PC
- Fanless IPC
- x86 workstation
- Future ARM industrial computers

NVision Edge may later become both:

1. Software product
2. NTech industrial hardware appliance

Example future product:

```text
NTech NVision Edge E100

- Industrial CPU/GPU
- 2–4 GigE Camera Ports
- Ethernet
- Digital I/O
- 24 VDC
- NVision Runtime
```

---

## 7.7 NVision Connect

**Purpose:** Industrial connectivity layer.

Initial protocol targets:

### PLC

- Siemens S7
- Mitsubishi MC Protocol
- Omron FINS
- Modbus TCP
- EtherNet/IP

### Robot

- TCP
- UDP
- REST
- gRPC
- Vendor-specific adapters

### Factory / IT

- OPC UA
- MQTT
- REST API
- WebSocket
- SQL
- MES integration

### Physical I/O

- Digital Input
- Digital Output
- Trigger Input
- Strobe Output

All industrial protocols should be implemented as adapters or plugins.

---

## 7.8 NVision SDK

**Purpose:** Allow external applications to embed or control NVision.

Possible SDKs:

```text
NVision .NET SDK
NVision C++ SDK
NVision Python SDK
NVision REST API
NVision gRPC API
```

Example conceptual API:

```csharp
var engine = new NVisionEngine();

engine.LoadProject("WireMarker.nvision");
engine.SelectRecipe("TYPE_01");

InspectionResult result =
    await engine.InspectAsync(frame);
```

Recommended internal namespaces:

```text
NTech.NVision.Core
NTech.NVision.Acquisition
NTech.NVision.Vision
NTech.NVision.AI
NTech.NVision.Workflow
NTech.NVision.Inspection
NTech.NVision.Calibration
NTech.NVision.Devices
NTech.NVision.Communication
NTech.NVision.Data
NTech.NVision.SDK
NTech.NVision.Studio
NTech.NVision.Runtime
```

---

## 7.9 NVision Hub

**Purpose:** Central management of multiple NVision inspection stations.

Concept:

```text
                     NVision Hub
                          │
          ┌───────────────┼───────────────┐
          ▼               ▼               ▼
      Station 01       Station 02      Station 03
      NVision Edge     NVision Edge    NVision Edge
```

Future capabilities:

- Station status
- Production status
- Yield
- NG rate
- Camera state
- PLC state
- Alarm monitoring
- Recipe deployment
- Model deployment
- Project deployment
- Version tracking
- Image history
- Production reports
- Central audit log
- User management

---

# 8. Core Domain Model

NVision should standardize the following concepts from the beginning.

## 8.1 Workspace

Optional container for multiple projects.

```text
Workspace
└── Projects
```

## 8.2 Project

A complete customer or machine Machine Vision application.

Example:

```text
WireMarkerInspection.nvision
```

A Project contains:

- Jobs
- Recipes
- Devices
- Models
- Calibration
- Resources
- Scripts
- Settings
- Deployment configuration

---

## 8.3 Job

A reusable inspection workflow.

Example:

```text
MarkerTypeInspection.njob
```

A project may contain multiple jobs.

Example:

```text
Project: Gear Inspection

Jobs
├── Top Inspection
├── Bottom Inspection
└── Side Inspection
```

---

## 8.4 Recipe

A product/SKU-specific parameter set.

Example:

```text
TYPE_01.recipe
TYPE_02.recipe
TYPE_03.recipe
TYPE_04.recipe
```

A recipe should override parameters without modifying the base inspection graph.

Possible recipe parameters:

- ROI
- Threshold
- Expected dimension
- OCR string
- Confidence
- AI model
- Product code
- Exposure
- Light intensity

---

## 8.5 Device

Physical or logical hardware connected to NVision.

Examples:

- Camera
- Light controller
- PLC
- Robot
- Barcode reader
- Digital I/O
- Trigger controller

---

## 8.6 Model

Versioned AI model package.

Example:

```text
WireMarkerClassifier
Version: 1.3.2
Backend: TensorRT
Input: 640x640
```

---

## 8.7 Inspection

One execution of a job against a product or trigger event.

Every Inspection must have a unique ID.

Example:

```text
InspectionId: LINE01-20260818-000012345
```

---

## 8.8 Inspection Result

Recommended base data model:

```json
{
  "inspectionId": "LINE01-20260818-000012345",
  "project": "WireMarkerInspection",
  "job": "MarkerTypeInspection",
  "recipe": "TYPE_01",
  "status": "OK",
  "score": 0.982,
  "cycleTimeMs": 83,
  "measurements": {},
  "detections": [],
  "alarms": [],
  "modelVersion": "1.3.2",
  "recipeVersion": "2.0.0",
  "timestamp": "2026-08-18T14:30:00+07:00"
}
```

Possible inspection statuses:

```text
OK
NG
ERROR
BYPASS
NO_RESULT
TIMEOUT
INVALID
```

`ERROR` and `NO_RESULT` must never be interpreted as `OK`.

---

# 9. Inspection Workflow Model

NVision Studio should be designed around a graph/node-based workflow.

Example:

```text
[Camera]
    │
    ▼
[Crop ROI]
    │
    ▼
[Normalize]
    │
    ▼
[AI Detect]
    │
    ▼
[Measure]
    │
    ▼
[Compare]
    │
    ▼
[Decision]
    │
    ├── OK
    └── NG
```

## 9.1 Node Categories

### Acquisition

```text
Camera
Image File
Image Folder
Video
Trigger
Frame Buffer
```

### Preprocessing

```text
Crop
Resize
Rotate
Perspective Transform
Undistort
Normalize
Blur
Sharpen
Color Convert
```

### Traditional Vision

```text
Threshold
Edge
Blob
Contour
Morphology
Template Matching
Pattern Matching
Circle
Line
Geometry
Measurement
Color Inspection
```

### AI

```text
Detect
Classify
Segment
OCR
Anomaly
Embedding
Similarity
```

### Logic

```text
Compare
AND
OR
NOT
Switch
Formula
Counter
Filter
State
```

### Data

```text
Variable
Constant
Recipe Parameter
Lookup Table
Barcode Input
Database Query
```

### Output

```text
Decision
Save Image
Save Result
PLC Write
Robot Command
Digital Output
MQTT Publish
OPC UA
REST
Database
```

---

# 10. Node Contract

Every workflow node should expose a common contract.

Conceptual interface:

```text
Node
│
├── Metadata
├── Inputs
├── Outputs
├── Parameters
├── Validation
├── Execute()
├── Diagnostics
└── Serialization
```

Each node should support:

- Unique Node ID
- Name
- Type
- Version
- Input ports
- Output ports
- Parameters
- Recipe-overridable parameters
- Validation
- Execution status
- Execution time
- Error detail
- Debug visualization

Node execution statuses:

```text
Idle
Ready
Running
Completed
Failed
Skipped
Disabled
```

---

# 11. Workflow Engine

The Workflow Engine is responsible for:

- Graph validation
- Dependency resolution
- Execution order
- Parallel execution where safe
- Cancellation
- Timeout
- Node error propagation
- Result propagation
- Performance measurement
- Debug stepping
- Simulation
- Serialization

The engine should eventually support both:

```text
Sequential Flow
Parallel Branches
Conditional Branches
```

Example:

```text
                    ┌─ [OCR] ─────────┐
[Camera] → [ROI] ───┤                 ├─ [Decision]
                    └─ [Measure] ─────┘
```

---

# 12. Camera Abstraction

Camera integration must be vendor-independent.

Recommended abstraction:

```text
ICamera
│
├── Connect()
├── Disconnect()
├── Start()
├── Stop()
├── Trigger()
├── Grab()
├── SetParameter()
├── GetParameter()
└── GetHealth()
```

Common camera parameters:

- Exposure
- Gain
- Frame rate
- Trigger mode
- Trigger source
- Pixel format
- Width
- Height
- Offset
- Packet size
- White balance

Potential adapters:

```text
Basler
Hikrobot
Daheng
FLIR
IDS
Allied Vision
USB/UVC
RTSP
Generic GigE Vision
GenICam
```

Vendor SDK dependencies must remain isolated from NVision Core.

---

# 13. Image Model

NVision should define a standard internal image representation.

Required metadata:

```text
Image
├── Width
├── Height
├── PixelFormat
├── Timestamp
├── CameraId
├── FrameId
├── Buffer
└── Metadata
```

Potential pixel formats:

```text
Mono8
Mono16
RGB8
BGR8
Bayer
Depth16
```

Avoid unnecessary image memory copies.

Where possible, design for:

- Buffer pooling
- Zero-copy
- GPU memory
- Shared frame ownership

---

# 14. AI Model Architecture

NVision should separate:

```text
Model Definition
Model Package
Inference Backend
Inference Session
Postprocessing
```

Recommended package metadata:

```json
{
  "name": "WireMarkerClassifier",
  "version": "1.3.2",
  "task": "classification",
  "inputWidth": 640,
  "inputHeight": 640,
  "classes": [
    "TYPE_1",
    "TYPE_2",
    "TYPE_3",
    "TYPE_4"
  ],
  "backend": "onnx",
  "checksum": "..."
}
```

AI inference interfaces should hide framework-specific details from workflow nodes.

Example:

```text
IAIModel
IAIInferenceSession
IAIBackend
```

Backends may include:

- ONNX Runtime
- TensorRT
- OpenVINO
- DirectML
- CUDA custom backend

---

# 15. Calibration

Calibration must be a first-class NVision concept.

Future calibration types:

```text
Camera Intrinsic Calibration
Lens Distortion Calibration
Pixel-to-mm Calibration
Perspective Calibration
Camera-to-Robot Calibration
Hand-Eye Calibration
Stereo Calibration
Multi-Camera Calibration
```

A calibration object should include:

- Calibration type
- Device IDs
- Calibration parameters
- Calibration date
- Operator
- Residual/error
- Version
- Validity status

Invalid or missing calibration must be visible in Runtime.

---

# 16. Inspection Decision Architecture

Inspection decision should be separated from individual Vision nodes.

Example:

```text
OCR Result
Measurement Result
AI Result
Presence Result
       │
       ▼
Decision Engine
       │
       ├── OK
       ├── NG
       └── ERROR
```

This allows product acceptance logic to evolve independently.

Typical rules:

```text
AIConfidence > 0.90
AND
Length BETWEEN 10.0 AND 10.5
AND
OCR == ExpectedText
```

---

# 17. Data Architecture

NVision data should be divided into:

## 17.1 Configuration Data

- Project
- Job
- Recipe
- Camera configuration
- PLC mapping
- User settings

## 17.2 Production Data

- Inspection results
- Counters
- Cycle time
- Yield
- NG reasons

## 17.3 Media Data

- Reference images
- Training images
- NG images
- Debug images

## 17.4 System Data

- Logs
- Alarms
- Diagnostics
- Audit trail

---

# 18. Storage Strategy

Initial local deployment may use:

```text
SQLite
+
File System
```

Future centralized deployment may use:

```text
PostgreSQL
+
Object Storage
```

Recommended local structure:

```text
data/
├── projects/
├── recipes/
├── models/
├── calibration/
├── images/
│   ├── reference/
│   ├── ok/
│   └── ng/
├── results/
├── logs/
└── diagnostics/
```

Retention must be configurable.

Example:

```text
Keep NG images: 180 days
Keep OK images: 7 days
Keep inspection results: 365 days
Keep logs: 90 days
```

---

# 19. Device State Model

Every external device should expose a common health state.

Suggested states:

```text
Unknown
Offline
Connecting
Online
Ready
Busy
Warning
Fault
Disabled
```

Example:

```text
Camera01    READY
Camera02    READY
PLC         ONLINE
Light01     READY
AI Engine   READY
```

Runtime must prevent production execution where required devices are not ready.

---

# 20. Alarm Model

Standard alarm structure:

```text
Alarm
├── Code
├── Severity
├── Source
├── Message
├── Timestamp
├── Acknowledged
└── Detail
```

Suggested severities:

```text
Info
Warning
Error
Critical
```

Example alarm codes:

```text
CAMERA_DISCONNECTED
CAMERA_TIMEOUT
PLC_COMMUNICATION_LOST
MODEL_LOAD_FAILED
RECIPE_INVALID
CALIBRATION_INVALID
DISK_SPACE_LOW
INSPECTION_TIMEOUT
```

---

# 21. User Roles

Initial role model:

## Operator

Allowed:

- Start
- Stop
- Select approved recipe
- Reset counter
- Acknowledge alarms

## Engineer

Allowed:

- Edit jobs
- Edit recipe
- Configure camera
- Debug inspection
- Calibration
- Publish project

## Administrator

Allowed:

- User management
- Licensing
- System configuration
- Backup/restore
- Security settings

Future:

```text
Viewer
Operator
Engineer
Administrator
Service
```

---

# 22. UI Architecture

NVision should separate at least two major UI modes.

## 22.1 Engineering UI

Product:

```text
NVision Studio
```

Layout concept:

```text
┌───────────────────────────────────────────────────────────┐
│ NVISION STUDIO                          Project / Recipe  │
├──────────────┬──────────────────────────────┬─────────────┤
│ TOOLBOX      │                              │ PROPERTIES  │
│              │       IMAGE / CANVAS         │             │
│ Acquisition  │                              │ Parameters  │
│ Vision       │                              │ Result      │
│ AI           │                              │ Debug       │
│ Logic        │                              │             │
│ Output       │                              │             │
├──────────────┴──────────────────────────────┴─────────────┤
│ WORKFLOW                                                  │
│ Camera → ROI → Detect → Measure → Decision → PLC         │
└───────────────────────────────────────────────────────────┘
```

## 22.2 Production UI

Product:

```text
NVision Runtime
```

Operator UI should emphasize:

- RUN / STOP
- Current product
- Current recipe
- Current image
- OK/NG
- Counters
- Yield
- Cycle time
- Device health
- Alarm

Avoid exposing unnecessary engineering controls.

---

# 23. Recommended Navigation

## NVision Studio

```text
PROJECT
├── Overview
├── Jobs
├── Recipes
└── Resources

DEVICES
├── Cameras
├── Lighting
├── PLC
├── Robot
└── I/O

VISION
├── Workflow
├── Calibration
├── AI Models
└── Reference Images

DATA
├── Inspection History
├── Images
└── Reports

DEPLOY
├── Runtime
├── Edge
└── Publish

SYSTEM
├── Logs
├── Diagnostics
├── Users
└── Settings
```

---

# 24. Deployment Package

Studio should publish a versioned deployment package.

Example:

```text
WireMarkerInspection-1.4.0.nvpkg
```

Possible content:

```text
package/
├── manifest.json
├── project.json
├── jobs/
├── recipes/
├── models/
├── calibration/
├── resources/
└── plugins/
```

Manifest example:

```json
{
  "product": "NVision",
  "project": "WireMarkerInspection",
  "version": "1.4.0",
  "runtimeMinVersion": "1.2.0",
  "createdAt": "2026-08-18T14:30:00+07:00"
}
```

Production machines should load only validated published packages.

---

# 25. Versioning

Use semantic versioning where appropriate:

```text
MAJOR.MINOR.PATCH
```

Example:

```text
NVision Studio 1.3.0
NVision Runtime 1.3.0
WireMarkerInspection 2.1.4
WireMarkerClassifier 1.5.2
```

Version separately:

- NVision platform
- Project
- Job schema
- Recipe
- Model
- Plugin
- Calibration

Inspection result should record relevant versions.

---

# 26. Plugin Architecture

NVision should eventually support plugin-based extensibility.

Plugin types:

```text
Camera Plugin
Vision Tool Plugin
AI Backend Plugin
PLC Plugin
Robot Plugin
Storage Plugin
Export Plugin
Workflow Node Plugin
```

Conceptual interface:

```text
INVisionPlugin
│
├── Id
├── Name
├── Version
├── Initialize()
├── Shutdown()
└── Capabilities
```

Plugins must be isolated enough that a failure does not corrupt the entire application state.

---

# 27. API Architecture

Recommended internal communication options:

### In-process

- Interfaces
- Events
- Message bus

### Local service

- gRPC
- Named pipe
- Unix socket

### Remote

- gRPC
- REST
- WebSocket

Recommended long-term structure:

```text
NVision Studio
      │
      ▼
NVision Service API
      │
      ▼
NVision Engine
```

Runtime UI may eventually be separable from the execution service.

This improves:

- Reliability
- Remote management
- Headless operation
- Edge deployment

---

# 28. Repository Architecture

Recommended monorepo structure:

```text
nvision/
│
├── apps/
│   ├── nvision-studio/
│   ├── nvision-runtime/
│   ├── nvision-trainer/
│   └── nvision-hub/
│
├── engine/
│   ├── core/
│   ├── acquisition/
│   ├── vision/
│   ├── ai/
│   ├── workflow/
│   ├── inspection/
│   ├── calibration/
│   ├── devices/
│   ├── communication/
│   ├── data/
│   └── diagnostics/
│
├── sdk/
│   ├── dotnet/
│   ├── cpp/
│   └── python/
│
├── plugins/
│   ├── cameras/
│   ├── vision/
│   ├── ai/
│   ├── plc/
│   ├── robot/
│   └── storage/
│
├── schemas/
│   ├── project/
│   ├── recipe/
│   ├── model/
│   └── result/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── hardware/
│   └── performance/
│
├── samples/
│
├── tools/
│
└── docs/
    ├── architecture/
    ├── product/
    ├── design/
    ├── protocols/
    ├── sdk/
    └── decisions/
```

---

# 29. Recommended Documentation Structure

```text
docs/
│
├── README.md
│
├── product/
│   ├── PRODUCT_VISION.md
│   ├── PRODUCT_ROADMAP.md
│   ├── USER_ROLES.md
│   └── TERMINOLOGY.md
│
├── architecture/
│   ├── PLATFORM_ARCHITECTURE.md
│   ├── ENGINE_ARCHITECTURE.md
│   ├── WORKFLOW_ENGINE.md
│   ├── PLUGIN_ARCHITECTURE.md
│   ├── DATA_ARCHITECTURE.md
│   └── DEPLOYMENT_ARCHITECTURE.md
│
├── design/
│   ├── UI_GUIDELINES.md
│   ├── DESIGN_SYSTEM.md
│   ├── COMPONENT_RULES.md
│   └── USER_FLOWS.md
│
├── ai/
│   ├── AI_ARCHITECTURE.md
│   ├── MODEL_FORMAT.md
│   └── DEPLOYMENT_BACKENDS.md
│
├── devices/
│   ├── CAMERA_ABSTRACTION.md
│   ├── PLC_INTEGRATION.md
│   └── ROBOT_INTEGRATION.md
│
├── sdk/
│
└── decisions/
    ├── ADR-0001-platform-first.md
    ├── ADR-0002-node-workflow.md
    └── ADR-0003-ai-backend-abstraction.md
```

---

# 30. Suggested Technology Direction

The exact stack may evolve. The architecture should not be tightly coupled to a single UI framework.

A reasonable initial direction for Windows industrial applications is:

```text
Language        C#
Platform        .NET
Desktop UI      WPF
Architecture    MVVM
Vision          OpenCV / OpenCvSharp
AI              ONNX Runtime / TensorRT
Database        SQLite
Logging         Serilog
API             gRPC / REST
```

Native C++ libraries may be added for:

- Performance-critical vision
- Camera SDK
- GPU processing
- Vendor APIs

Jetson/Edge runtime may use:

```text
C++
.NET where supported
Python only for non-critical tooling/training
TensorRT
CUDA
ONNX
```

Core design rule:

> UI technology must not define the architecture of NVision Engine.

---

# 31. Performance Requirements

Machine Vision software must measure performance continuously.

Every inspection should capture at least:

```text
Acquisition Time
Preprocessing Time
Vision Time
AI Inference Time
Decision Time
Communication Time
Total Cycle Time
```

Example:

```text
Acquisition      8 ms
Preprocess       5 ms
AI              28 ms
Measurement      7 ms
Decision         1 ms
PLC              3 ms
---------------------
Total           52 ms
```

Future profiler:

```text
NVision Performance Profiler
```

should expose timing per workflow node.

---

# 32. Reliability Requirements

Production Runtime should support:

- Watchdog
- Graceful restart
- Camera reconnect
- PLC reconnect
- Model validation
- Disk space monitoring
- Database recovery
- Controlled shutdown
- Crash dump
- Health endpoint
- Configuration backup
- Project rollback

Runtime states:

```text
STARTING
INITIALIZING
READY
RUNNING
STOPPED
WARNING
FAULT
MAINTENANCE
```

---

# 33. Security Baseline

Even for offline industrial systems, NVision should plan for:

- Role-based access
- Password protection
- Audit log
- Signed deployment packages
- Model integrity verification
- Configuration integrity
- Secure API authentication
- TLS for remote communication
- Backup encryption where required

Never store credentials directly in project files.

---

# 34. Licensing Direction

Licensing does not need to be implemented in the MVP, but architecture should not block it.

Possible license levels:

```text
NVision Runtime
NVision Studio
NVision AI
NVision Edge
NVision Hub
NVision SDK
```

Possible activation models:

- Machine license
- USB dongle
- Offline activation file
- Floating license
- Enterprise license

Industrial customers may operate in networks without Internet connectivity, so offline licensing should be supported.

---

# 35. Testing Strategy

Testing layers:

## Unit Tests

Test:

- Vision functions
- Decision logic
- Serialization
- Recipe resolution
- Workflow validation

## Integration Tests

Test:

- Camera adapter
- PLC adapter
- AI backend
- Database
- Runtime service

## Hardware Tests

Use real:

- Camera
- PLC
- Trigger
- GPU
- Robot

## Performance Tests

Measure:

- FPS
- Cycle time
- Memory
- CPU
- GPU
- Long-running stability

## Golden Image Tests

Store fixed reference images and expected results.

Example:

```text
tests/golden/
├── wire_marker/
├── gear/
└── label/
```

Golden image regression testing is especially important for Machine Vision.

---

# 36. Traceability

Every production result should be traceable to:

```text
Project Version
Job Version
Recipe Version
Model Version
Calibration Version
Runtime Version
Camera
Timestamp
Inspection ID
```

This makes root-cause analysis possible when a customer reports a false OK or false NG.

---

# 37. Backup and Restore

NVision should eventually support a standard backup package containing:

- Project
- Recipe
- Models
- Calibration
- Configuration
- PLC mappings

Production data may optionally be excluded.

Example:

```text
NVisionBackup-Station01-20260818.nvbackup
```

---

# 38. Example: Wire Marker Inspection

A customer application should be represented as:

```text
NVision Platform
└── Project: Wire Marker Inspection
    │
    ├── Job: Marker Type Inspection
    │
    ├── Recipes
    │   ├── TYPE_01
    │   ├── TYPE_02
    │   ├── TYPE_03
    │   └── TYPE_04
    │
    ├── Cameras
    │   └── Camera01
    │
    ├── Models
    │   └── MarkerClassifier
    │
    └── PLC Mapping
```

Workflow example:

```text
[Camera]
   ↓
[Locate Marker]
   ↓
[Normalize Orientation]
   ↓
[OCR / Feature Extraction]
   ↓
[Classification]
   ↓
[Compare With Active Recipe]
   ↓
[Decision]
   ├── OK
   └── NG
```

The inspection application belongs to the project.

The reusable tools belong to NVision.

---

# 39. Example: Gear Inspection

```text
Project: GearInspection

Jobs
├── TopInspection
├── BottomInspection
└── SideInspection

Recipes
├── Gear_A
├── Gear_B
└── Gear_C
```

Workflow:

```text
Camera
 ↓
Part Localization
 ↓
ROI
 ↓
AI Defect Detection
 ↓
Geometric Measurement
 ↓
Rule Evaluation
 ↓
OK / NG
```

Again:

> Gear inspection is an NVision application, not NVision itself.

---

# 40. Product Roadmap

## Phase 0 — Foundation

Objectives:

- Naming
- Product definition
- Architecture baseline
- Repository
- Documentation
- Coding rules
- UI direction

Deliverables:

```text
NVision Platform Foundation
Architecture docs
Repo skeleton
Design system
Core interfaces
```

---

## Phase 1 — NVision Core MVP

Products:

```text
NVision Studio
NVision Runtime
NVision Engine
```

Required capabilities:

- Camera
- Image acquisition
- ROI
- Basic preprocessing
- Basic OpenCV tools
- Workflow engine
- Recipe
- Inspection Result
- PLC communication
- Result history
- Logging
- Image saving
- Runtime mode

Primary goal:

> Build and deploy a real production inspection without customer-specific architecture changes.

---

## Phase 2 — NVision AI

Add:

- ONNX model
- TensorRT
- Detection
- Classification
- Segmentation
- AI model manager
- Model versioning
- GPU profiling

---

## Phase 3 — Advanced Vision

Add:

- Calibration
- Measurement
- Pattern matching
- OCR
- Barcode
- Multi-camera
- Parallel workflow
- Advanced debug tools

---

## Phase 4 — NVision Edge

Add:

- Jetson deployment
- Headless Runtime
- Remote API
- Hardware health
- Edge service
- Industrial appliance support

---

## Phase 5 — NVision Trainer

Add:

- Dataset manager
- Annotation
- Training
- Evaluation
- Export
- Model registry

---

## Phase 6 — NVision Hub

Add:

- Multi-station management
- Central deployment
- Dashboard
- Production analytics
- Alarm monitoring
- Image search
- Audit

---

## Phase 7 — NVision Ecosystem

Add:

- SDK
- Plugin API
- Partner integrations
- Solution templates
- Marketplace
- Enterprise management

---

# 41. MVP Scope Recommendation

Do not attempt to implement the complete ecosystem in the first version.

Recommended first production-capable architecture:

```text
NVision Studio
      │
      ▼
NVision Engine
      │
      ├── Camera
      ├── Vision
      ├── AI
      ├── Workflow
      ├── Recipe
      ├── PLC
      └── Data
      │
      ▼
NVision Runtime
```

Minimum node set:

```text
Camera
Image
Crop
Resize
Rotate
Threshold
Edge
Blob
Contour
AI Detect
AI Classify
OCR
Compare
AND
OR
Decision
Save Image
PLC Write
```

This is enough to build many industrial applications.

---

# 42. Non-Goals for Initial MVP

Avoid building too early:

- Cloud dependency
- Marketplace
- Complex multi-user enterprise management
- Full AI training framework
- Dozens of camera vendors
- Dozens of PLC protocols
- Microservice architecture everywhere
- Custom scripting language
- Web UI for every component

Focus first on a stable industrial inspection engine.

---

# 43. Engineering Decision Rules

When deciding whether a feature belongs in the platform, ask:

### Question 1

Can this feature be reused by another Machine Vision project?

If yes:

> Put it in NVision.

### Question 2

Is this specific to one customer, product, or SKU?

If yes:

> Put it in Project / Job / Recipe.

### Question 3

Does it depend on one hardware vendor?

If yes:

> Put it behind an Adapter / Plugin.

### Question 4

Does Studio require it but Runtime does not?

If yes:

> Keep the execution contract in Engine and editing UI in Studio.

### Question 5

Does AI backend implementation leak into workflow logic?

If yes:

> Refactor behind AI abstraction.

---

# 44. Naming Rules

Use consistent product names:

```text
NTech NVision
NVision Studio
NVision Runtime
NVision Engine
NVision AI
NVision Trainer
NVision Edge
NVision Connect
NVision SDK
NVision Hub
```

Avoid inconsistent names such as:

```text
N-Vision
N Vision
NVisionAIPlatform
NTechVisionSoftware
```

Recommended executable/service names:

```text
NVision.Studio
NVision.Runtime
NVision.Service
NVision.Hub
```

Recommended package naming:

```text
NTech.NVision.*
```

---

# 45. File Extensions

Proposed NVision file formats:

```text
.nvision    NVision Project
.njob       Inspection Job
.nrecipe    Recipe
.nvmodel    AI Model Package
.nvcal      Calibration
.nvpkg      Deployment Package
.nvbackup   Backup Package
```

These extensions are conceptual and may change before public release.

Use JSON/YAML internally where convenient before designing custom binary formats.

---

# 46. Branding Language

Recommended master tagline:

> **Industrial Vision Intelligence**

Recommended Studio tagline:

> **Build Vision. Deploy Intelligence.**

Recommended technical description:

> **Machine Vision + AI + Industrial Automation**

Recommended short explanation:

> **NVision transforms industrial camera data into intelligent inspection decisions.**

Vietnamese:

> **NVision biến dữ liệu hình ảnh công nghiệp thành quyết định kiểm tra thông minh.**

---

# 47. Architecture Summary

```text
                           ┌─────────────────┐
                           │ NVision Studio  │
                           └────────┬────────┘
                                    │ Build / Publish
                                    ▼
                           ┌─────────────────┐
                           │ Deployment Pkg  │
                           └────────┬────────┘
                                    │
                                    ▼
┌─────────┐               ┌─────────────────┐              ┌─────────┐
│ Camera  │──────────────►│ NVision Runtime │─────────────►│  PLC    │
└─────────┘               └────────┬────────┘              └─────────┘
                                   │
                                   ▼
                         ┌─────────────────────┐
                         │   NVision Engine    │
                         ├─────────────────────┤
                         │ Acquisition         │
                         │ Vision              │
                         │ AI                  │
                         │ Workflow            │
                         │ Inspection          │
                         │ Calibration         │
                         │ Connect             │
                         │ Data                │
                         └─────────────────────┘
                                   │
               ┌───────────────────┼───────────────────┐
               ▼                   ▼                   ▼
             Robot                MES              Database
```

---

# 48. Long-Term Platform Vision

The desired progression is:

```text
Stage 1
Customer-specific Vision Software

        ↓

Stage 2
Reusable NVision Engine

        ↓

Stage 3
NVision Studio + Runtime

        ↓

Stage 4
NVision Edge + AI

        ↓

Stage 5
NVision Hub

        ↓

Stage 6
NVision Industrial Vision Ecosystem
```

The long-term strategic asset is not any individual inspection project.

The strategic asset is:

> **The reusable NVision platform that allows NTech to build future Machine Vision applications faster, more reliably, and with increasingly less project-specific code.**

---

# 49. Definition of Success

NVision is successful when a new inspection project can be implemented primarily by:

1. Creating a Project
2. Connecting devices
3. Building a workflow
4. Creating Recipes
5. Loading or training AI models
6. Configuring acceptance rules
7. Mapping PLC/Robot outputs
8. Publishing to Runtime

instead of writing a new application from scratch.

Target future workflow:

```text
New Customer
     │
     ▼
Create NVision Project
     │
     ▼
Configure Hardware
     │
     ▼
Build Inspection Flow
     │
     ▼
Configure Recipes
     │
     ▼
Validate
     │
     ▼
Publish
     │
     ▼
Production
```

---

# 50. Final Platform Statement

# NTech NVision

> **Industrial Vision Intelligence**

NVision is NTech's reusable industrial Machine Vision platform combining:

- Image acquisition
- Traditional Computer Vision
- AI
- Measurement
- Inspection logic
- Industrial communication
- Runtime execution
- Data traceability
- Edge deployment
- Factory management

The foundational architectural rule is:

> **Build the platform once. Configure inspection applications many times.**

And the product rule is:

> **A customer inspection is a Project running on NVision — it is not NVision itself.**

---

# 51. Immediate Next Documents

After adopting this foundation, the following documents should be created next:

```text
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
```

Recommended implementation order:

```text
Foundation
   ↓
Domain Model
   ↓
Engine Interfaces
   ↓
Workflow Engine
   ↓
Camera Abstraction
   ↓
Inspection Result
   ↓
Studio MVP
   ↓
Runtime MVP
   ↓
AI Engine
   ↓
Industrial Connectivity
```

---

**End of Document**
