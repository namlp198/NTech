#include "crop_tool.hpp"

#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/execution_result.hpp"
#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"
#include "nvision/workflow/parameter_schema.hpp"
#include "nvision/workflow/port_value.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using core::AxisAlignedRectangle;
using core::ErrorCategory;
using core::ExecutionError;
using workflow::GeometryKind;
using core::ParameterKey;
using core::PortKey;
using core::ResultCode;
using core::SemanticVersion;
using core::ToolTypeId;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;
using workflow::ToolImplementation;

ExecutionError configuration_error(std::string code, std::string detail) {
    return ExecutionError(
        ResultCode(std::move(code)), ErrorCategory::configuration, std::move(detail), false);
}

// A region of interest is only meaningful together with the space it was authored in.
// Silently reinterpreting it against a differently cropped view would shift the result.
bool describes_same_space(const ImageView& image, const AxisAlignedRectangle& roi) {
    return roi.coordinate_space() == image.coordinate_space();
}

bool is_whole_pixel(const double value) noexcept {
    return std::isfinite(value) && value >= 0.0 &&
           value <= static_cast<double>(std::numeric_limits<std::uint32_t>::max()) &&
           std::floor(value) == value;
}

class CropTool final : public ToolImplementation {
public:
    CropTool() : id_("nvision.tool.processing.crop"), version_(1U, 0U, 0U) {}

    [[nodiscard]] const ToolTypeId& tool_type_id() const noexcept override {
        return id_;
    }

    [[nodiscard]] const SemanticVersion& tool_version() const noexcept override {
        return version_;
    }

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const auto& roi_value = context.parameter(ParameterKey("roi"));

        if (roi_value.geometry_kind() != GeometryKind::rectangle) {
            // The catalog also allows polygon and circle regions. Their crop semantics
            // arrive with the masking tools rather than being approximated here.
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_ROI_SHAPE",
                "crop currently supports a rectangular region of interest only"));
        }

        const auto& roi = std::get<AxisAlignedRectangle>(roi_value.geometry_value());
        if (!describes_same_space(image, roi)) {
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the region of interest was authored in coordinate space " +
                    roi.coordinate_space().value() + " but the input image is in " +
                    image.coordinate_space().value()));
        }

        if (!is_whole_pixel(roi.x()) || !is_whole_pixel(roi.y()) ||
            !is_whole_pixel(roi.width()) || !is_whole_pixel(roi.height())) {
            return ToolExecutionOutcome::failed(configuration_error(
                "ROI_NOT_PIXEL_ALIGNED",
                "crop requires a region of interest on whole pixel boundaries"));
        }

        const auto origin_x = static_cast<std::uint32_t>(roi.x());
        const auto origin_y = static_cast<std::uint32_t>(roi.y());
        const auto width = static_cast<std::uint32_t>(roi.width());
        const auto height = static_cast<std::uint32_t>(roi.height());
        if (width == 0U || height == 0U || origin_x >= image.width() ||
            origin_y >= image.height() || width > image.width() - origin_x ||
            height > image.height() - origin_y) {
            return ToolExecutionOutcome::failed(configuration_error(
                "ROI_OUT_OF_BOUNDS",
                "the region of interest lies outside the input image view"));
        }

        ImageView cropped = image.subview(origin_x, origin_y, width, height);
        core::Transform2D view_to_source = cropped.view_to_source();

        PortValueMap outputs;
        outputs.set(PortKey("image"), PortValue::image(std::move(cropped)));
        outputs.set(PortKey("transform"), PortValue::transform(std::move(view_to_source)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }

private:
    ToolTypeId id_;
    SemanticVersion version_;
};

}  // namespace

std::shared_ptr<const ToolImplementation> create_crop_tool() {
    return std::make_shared<const CropTool>();
}

}  // namespace nvision::vision
