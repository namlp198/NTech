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
