# ADR-0007 — Computed images are a separate raster kind from captured frames

- **Status:** Accepted
- **Date:** 2026-08-27
- **Related:** ADR-0005, ADR-0006, `docs/implementation/16_GENERIC_VISION_TOOL_PLATFORM_EXECUTION_PLAN.md` Sections 10 and 15

## Context

Until M3 every raster in the native plane came from a camera. `Frame` is the captured
raster: it carries a frame identifier, a camera identifier, a sequence number, capture and
receive timestamps, a delivery status, and a timestamp quality. `ImageView` is a
non-owning window onto a `Frame` that retains its lease, so pixels outlive every view.

The first implemented tool, Crop, fitted that model exactly: it produces another view onto
the same captured pixels. The rest of the M3 image-processing group does not. Resize,
colour conversion, blur, threshold, normalise, morphology, distance transform, and the
warps all **produce new pixels**, and there is no `Frame` for those pixels to belong to.

The available options were:

1. Let a tool fabricate a `Frame` for its output. Every field would have to be invented: a
   frame identifier, a camera identifier, a sequence number, and capture timestamps.
2. Widen `Frame` with a "derived" mode in which the acquisition metadata is absent.
3. Introduce a second raster kind that owns its pixels and carries no acquisition
   metadata at all.

Option 1 makes a computed raster indistinguishable from a real capture in results,
evidence, and traceability — precisely the confusion the platform boundary exists to
prevent. Option 2 gives one type two meanings, so every reader of frame metadata has to
check which meaning it is holding, and nothing in the type system enforces that check.

## Decision

A computed raster is a distinct type.

```text
Frame     captured pixels + acquisition metadata      (camera, sequence, timestamps, status)
Image     computed pixels + stable identity           (no acquisition metadata at all)
              ▲                       ▲
              └──────── ImageSource ──┘
                             ▲
                        ImageView       a window onto either, retaining its lease
```

- `nvision::acquisition::Image` is immutable, owns its bytes, and validates its own
  dimensions, stride, and buffer length. It has no camera, no sequence number, and no
  timestamps, because a computed raster has none.
- Its identity is supplied by the producer as a `core::ImageId`, not generated, so a
  repeated run derives the same coordinate space for the same output.
  `ToolExecutionContext::output_image_id` derives that identity from the workflow node and
  the output port for exactly this reason.
- `ImageSource` names which raster kind owns the pixels a view exposes.
  `ImageView::source().is_captured()` answers it, and `ImageView::frame()` returns a null
  lease for a computed image rather than a fabricated one.
- Coordinate spaces stay symmetric: `nvision.frame.<hex-id>.image-pixel` for a capture and
  `nvision.image.<identity>.image-pixel` for a computed raster, with subviews named by
  their absolute position in the source raster so nesting cannot grow the identifier
  without bound.

Both kinds live in `nvision::acquisition`, which owns the pixel-buffer vocabulary —
`PixelFormat`, buffers, and views. `Frame` is the captured raster and `Image` is the
computed one; neither is a workflow or vision concept, and duplicating the buffer
vocabulary into another area to host one type would be worse than the naming stretch.

## Consequences

- Every M3 tool that materialises an image produces an `Image`, wraps it in an
  `ImageView`, and hands that to the typed port. `PortValue::image` needs no change,
  because it already carries an `ImageView`.
- Provenance survives a pipeline. A view derived from a capture still answers
  `frame()`, and a view over a computed raster cannot pretend it was captured.
- `ImageView` gained an `ImageSource` and a `source()` accessor, and `frame()` now returns
  a lease by value that may be null. The versioned C ABI is unaffected: it exposes
  `nv_frame` and never exposed `ImageView`.
- A tool that only narrows an existing raster must keep producing a view, not a copy.
  Crop still does, and that remains the expected shape for any purely geometric operation.
- Deriving image identity from the node and the port means two invocations of the same
  node within one run reuse an identity. That is correct for the current acyclic graphs.
  A future looping or batching construct must extend the derivation rather than rely on
  the current form.

## Alternatives considered

- **Fabricate a `Frame` per computed image.** Rejected: it forges acquisition provenance.
- **A "derived" mode on `Frame`.** Rejected: one type with two meanings and no
  type-level enforcement of which one a caller holds.
- **Type-erase the owner behind `shared_ptr<const void>`.** Rejected: it makes every view
  cheap to construct but destroys the ability to ask where the pixels came from, which
  results and evidence need.
