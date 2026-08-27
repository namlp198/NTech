#pragma once

// This header is internal to nvision_vision. It is the only place where NVision image
// contracts meet OpenCV types, and it must never be reachable from a public header.

#include "nvision/acquisition/image.hpp"
#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/identifier.hpp"

#include <opencv2/core.hpp>

namespace nvision::vision::interop {

// The OpenCV element type that matches a pixel format, or a throw when the format has no
// OpenCV equivalent yet.
[[nodiscard]] int opencv_type_of(acquisition::PixelFormat format);

// Wraps the pixels a view exposes without copying them. The result borrows the raster the
// view keeps alive, so it must not outlive that view, and callers must treat it as
// read-only: NVision rasters are immutable.
[[nodiscard]] cv::Mat borrow(const acquisition::ImageView& view);

// Copies an OpenCV result into an NVision-owned computed image. The copy is deliberate:
// an owned raster must not depend on memory OpenCV may release or reallocate.
[[nodiscard]] acquisition::ImageLease materialize(core::ImageId id, const cv::Mat& produced);

}  // namespace nvision::vision::interop
