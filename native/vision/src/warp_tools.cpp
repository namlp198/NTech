#include "warp_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/geometry.hpp"

#include <opencv2/imgproc.hpp>

#include <array>
#include <string>
#include <utility>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using core::ParameterKey;
using core::PortKey;
using core::Transform2D;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

cv::Matx33d as_matrix(const Transform2D& transform) {
    const std::array<double, 9>& values = transform.matrix();
    return cv::Matx33d(
        values[0], values[1], values[2],
        values[3], values[4], values[5],
        values[6], values[7], values[8]);
}

std::array<double, 9> as_array(const cv::Matx33d& matrix) {
    return std::array<double, 9>{
        matrix(0, 0), matrix(0, 1), matrix(0, 2),
        matrix(1, 0), matrix(1, 1), matrix(1, 2),
        matrix(2, 0), matrix(2, 1), matrix(2, 2)};
}

int interpolation_of(const std::string& name) noexcept {
    if (name == "nearest") {
        return cv::INTER_NEAREST;
    }
    if (name == "cubic") {
        return cv::INTER_CUBIC;
    }
    if (name == "area") {
        return cv::INTER_AREA;
    }
    return cv::INTER_LINEAR;
}

// The coordinate space a tool's produced image will carry, needed before the image exists
// so the accompanying transform can name its source space.
core::CoordinateSpaceId produced_space(
    const ToolExecutionContext& context,
    const PortKey& output_port) {
    return core::CoordinateSpaceId(
        "nvision.image." + context.output_image_id(output_port).value() + ".image-pixel");
}

class ResizeTool final : public VisionTool {
public:
    ResizeTool() : VisionTool("nvision.tool.processing.resize", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const auto width = static_cast<int>(
            context.parameter(ParameterKey("width")).integer_value());
        const auto height = static_cast<int>(
            context.parameter(ParameterKey("height")).integer_value());
        const std::string interpolation =
            context.parameter(ParameterKey("interpolation")).enumeration_value().value();

        cv::Mat resized;
        cv::resize(
            interop::borrow(image),
            resized,
            cv::Size(width, height),
            0.0,
            0.0,
            interpolation_of(interpolation));

        // Resizing changes what a pixel coordinate means, so the scale back to the input
        // space is published rather than left for a downstream node to reconstruct.
        const double scale_x = static_cast<double>(image.width()) / static_cast<double>(width);
        const double scale_y = static_cast<double>(image.height()) / static_cast<double>(height);
        const Transform2D to_source(
            produced_space(context, PortKey("image")),
            image.coordinate_space(),
            std::array<double, 9>{scale_x, 0.0, 0.0, 0.0, scale_y, 0.0, 0.0, 0.0, 1.0});

        PortValueMap outputs;
        outputs.set(
            PortKey("image"),
            produced_image(context, PortKey("image"), resized, image.pixel_format()));
        outputs.set(PortKey("transform"), PortValue::transform(to_source));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class PerspectiveWarpTool final : public VisionTool {
public:
    PerspectiveWarpTool()
        : VisionTool("nvision.tool.processing.perspective-warp", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const Transform2D& homography =
            context.input(PortKey("homography")).transform_value();

        if (homography.source_space() != image.coordinate_space()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the homography maps from " + homography.source_space().value() +
                    " but the input image is in " + image.coordinate_space().value()));
        }

        auto width = static_cast<int>(context.parameter(ParameterKey("width")).integer_value());
        auto height = static_cast<int>(context.parameter(ParameterKey("height")).integer_value());
        // Zero means keep the input size, which is the common case of rectifying in place.
        if (width == 0) {
            width = static_cast<int>(image.width());
        }
        if (height == 0) {
            height = static_cast<int>(image.height());
        }

        // Transform2D rejects a non-invertible matrix at construction, so the inverse
        // always exists. Should the numerical inversion still degrade, the Transform2D
        // built from it below refuses the result and the run reports a technical fault
        // rather than publishing a mapping that does not map back.
        const cv::Matx33d matrix = as_matrix(homography);
        const cv::Matx33d inverse = matrix.inv(cv::DECOMP_LU);

        cv::Mat warped;
        cv::warpPerspective(interop::borrow(image), warped, cv::Mat(matrix), cv::Size(width, height));

        const Transform2D to_source(
            produced_space(context, PortKey("image")),
            image.coordinate_space(),
            as_array(inverse));

        PortValueMap outputs;
        outputs.set(
            PortKey("image"),
            produced_image(context, PortKey("image"), warped, image.pixel_format()));
        outputs.set(PortKey("transform"), PortValue::transform(to_source));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class AffineTransformTool final : public VisionTool {
public:
    AffineTransformTool()
        : VisionTool("nvision.tool.processing.affine-transform", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const Transform2D& transform = context.input(PortKey("transform")).transform_value();

        if (transform.source_space() != image.coordinate_space()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the transform maps from " + transform.source_space().value() +
                    " but the input image is in " + image.coordinate_space().value()));
        }

        const std::array<double, 9>& values = transform.matrix();
        if (values[6] != 0.0 || values[7] != 0.0 || values[8] != 1.0) {
            // A projective matrix on an affine port would silently drop its perspective
            // component, so it is refused and sent to the perspective warp instead.
            return ToolExecutionOutcome::failed(configuration_error(
                "TRANSFORM_NOT_AFFINE",
                "the transform has a projective component; use perspective warp"));
        }

        const cv::Matx23d affine(
            values[0], values[1], values[2],
            values[3], values[4], values[5]);
        cv::Mat warped;
        cv::warpAffine(
            interop::borrow(image),
            warped,
            cv::Mat(affine),
            cv::Size(static_cast<int>(image.width()), static_cast<int>(image.height())));

        PortValueMap outputs;
        outputs.set(
            PortKey("image"),
            produced_image(context, PortKey("image"), warped, image.pixel_format()));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_resize_tool() {
    return std::make_shared<const ResizeTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_perspective_warp_tool() {
    return std::make_shared<const PerspectiveWarpTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_affine_transform_tool() {
    return std::make_shared<const AffineTransformTool>();
}

}  // namespace nvision::vision
