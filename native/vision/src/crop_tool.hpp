#pragma once

#include "nvision/workflow/tool_execution.hpp"

#include <memory>

namespace nvision::vision {

// Crop restricts an image to a region of interest and publishes the transform back to
// the source coordinate space. It never copies pixels: the produced value is another
// view that retains the same frame lease.
[[nodiscard]] std::shared_ptr<const workflow::ToolImplementation> create_crop_tool();

}  // namespace nvision::vision
