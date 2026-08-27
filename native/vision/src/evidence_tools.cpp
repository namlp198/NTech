#include "evidence_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/overlay.hpp"

#include <opencv2/imgproc.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::AxisAlignedRectangle;
using core::Circle;
using core::Contour;
using core::EvidenceArtifact;
using core::EvidenceKind;
using core::Line2D;
using core::Overlay;
using core::OverlayItem;
using core::ParameterKey;
using core::Point2D;
using core::Polygon;
using core::PortKey;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

cv::Point rounded(const Point2D& point) noexcept {
    return cv::Point(static_cast<int>(std::lround(point.x())), static_cast<int>(std::lround(point.y())));
}

std::vector<cv::Point> rounded_all(const std::vector<Point2D>& points) {
    std::vector<cv::Point> converted;
    converted.reserve(points.size());
    for (const Point2D& point : points) {
        converted.push_back(rounded(point));
    }
    return converted;
}

// OpenCV orders its channels blue first, so a style's colour is reversed on the way out.
cv::Scalar colour_of(const core::OverlayStyle& style) noexcept {
    return cv::Scalar(
        static_cast<double>(style.blue()),
        static_cast<double>(style.green()),
        static_cast<double>(style.red()));
}

int stroke_of(const core::OverlayStyle& style) noexcept {
    return style.filled() ? cv::FILLED : std::max(1, static_cast<int>(std::lround(style.thickness())));
}

void draw(cv::Mat& canvas, const OverlayItem& item) {
    const cv::Scalar colour = colour_of(item.style());
    const int stroke = stroke_of(item.style());

    std::visit(
        [&canvas, &colour, stroke](const auto& shape) {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, Point2D>) {
                cv::circle(canvas, rounded(shape), std::max(1, stroke), colour, cv::FILLED);
            } else if constexpr (std::is_same_v<Shape, Line2D>) {
                cv::line(canvas, rounded(shape.start()), rounded(shape.end()), colour, stroke);
            } else if constexpr (std::is_same_v<Shape, Circle>) {
                cv::circle(
                    canvas,
                    rounded(shape.center()),
                    static_cast<int>(std::lround(shape.radius())),
                    colour,
                    stroke);
            } else if constexpr (std::is_same_v<Shape, AxisAlignedRectangle>) {
                cv::rectangle(
                    canvas,
                    cv::Rect(
                        static_cast<int>(std::lround(shape.x())),
                        static_cast<int>(std::lround(shape.y())),
                        static_cast<int>(std::lround(shape.width())),
                        static_cast<int>(std::lround(shape.height()))),
                    colour,
                    stroke);
            } else if constexpr (std::is_same_v<Shape, Polygon>) {
                const std::vector<cv::Point> vertices = rounded_all(shape.vertices());
                if (stroke == cv::FILLED) {
                    cv::fillPoly(canvas, std::vector<std::vector<cv::Point>>{vertices}, colour);
                } else {
                    cv::polylines(canvas, vertices, true, colour, stroke);
                }
            } else {
                const std::vector<cv::Point> points = rounded_all(shape.points());
                cv::polylines(canvas, points, shape.closed(), colour, stroke);
            }
        },
        item.shape());

    if (!item.label().empty()) {
        const cv::Point anchor = rounded(item.anchor());
        cv::putText(
            canvas,
            item.label(),
            cv::Point(anchor.x, anchor.y - 4),
            cv::FONT_HERSHEY_SIMPLEX,
            0.4,
            colour,
            1);
    }
}

// The pixels a view exposes, packed without the stride of the raster behind it, which is
// what has to be recorded because a stride is an artefact of storage rather than content.
std::vector<std::uint8_t> packed_bytes(const ImageView& view) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(
        static_cast<std::size_t>(view.height()) *
        static_cast<std::size_t>(view.width()) *
        acquisition::bytes_per_pixel(view.pixel_format()));
    for (std::uint32_t row = 0U; row < view.height(); ++row) {
        const auto line = view.row(row);
        bytes.insert(bytes.end(), line.begin(), line.end());
    }
    return bytes;
}

std::vector<std::uint8_t> packed_bytes(const cv::Mat& rendered) {
    std::vector<std::uint8_t> bytes;
    const std::size_t row_bytes =
        static_cast<std::size_t>(rendered.cols) * rendered.elemSize();
    bytes.reserve(row_bytes * static_cast<std::size_t>(rendered.rows));
    for (int row = 0; row < rendered.rows; ++row) {
        const std::uint8_t* line = rendered.ptr<std::uint8_t>(row);
        bytes.insert(bytes.end(), line, line + row_bytes);
    }
    return bytes;
}

class EvidenceOutputTool final : public VisionTool {
public:
    EvidenceOutputTool()
        : VisionTool("nvision.tool.logic.evidence-output", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const PortValue* annotation = context.inputs().find(PortKey("overlay"));

        if (annotation != nullptr &&
            annotation->overlay_value().coordinate_space() != image.coordinate_space()) {
            // Drawing a shape authored against another raster would place it somewhere it
            // never was, and the evidence would then misrepresent the inspection.
            return ToolExecutionOutcome::failed(configuration_error(
                "COORDINATE_SPACE_MISMATCH",
                "the overlay was authored in " +
                    annotation->overlay_value().coordinate_space().value() +
                    " but the image is in " + image.coordinate_space().value()));
        }

        workflow::EvidenceSink* sink = context.evidence_sink();
        if (sink == nullptr) {
            return ToolExecutionOutcome::failed(core::ExecutionError(
                core::ResultCode("EVIDENCE_SINK_UNAVAILABLE"),
                core::ErrorCategory::storage,
                "no evidence store is attached to this run, so the artifact could not be "
                "recorded",
                false));
        }

        std::vector<std::uint8_t> content;
        EvidenceKind kind = image.source().is_captured() ? EvidenceKind::source_image
                                                         : EvidenceKind::derived_image;
        if (annotation == nullptr) {
            content = packed_bytes(image);
        } else {
            // Annotation needs colour, so a single-channel image is widened first. The
            // drawing happens on a copy: an NVision raster is immutable.
            cv::Mat canvas;
            if (image.pixel_format() == PixelFormat::bgr8) {
                canvas = interop::borrow(image).clone();
            } else if (image.pixel_format() == PixelFormat::mono8) {
                cv::cvtColor(interop::borrow(image), canvas, cv::COLOR_GRAY2BGR);
            } else {
                return ToolExecutionOutcome::failed(configuration_error(
                    "UNSUPPORTED_PIXEL_FORMAT",
                    "annotated evidence requires an 8-bit grey or BGR image"));
            }
            for (const OverlayItem& item : annotation->overlay_value().items()) {
                draw(canvas, item);
            }
            content = packed_bytes(canvas);
            kind = EvidenceKind::overlay;
        }

        std::string storage_key =
            context.parameter(ParameterKey("storage-key")).string_value();
        if (storage_key.empty()) {
            storage_key = context.node_id().value();
        }

        PortValueMap outputs;
        outputs.set(
            PortKey("evidence"),
            PortValue::evidence(EvidenceArtifact(
                context.output_evidence_id(PortKey("evidence")),
                kind,
                sink->store(storage_key, content),
                image.coordinate_space())));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_evidence_output_tool() {
    return std::make_shared<const EvidenceOutputTool>();
}

}  // namespace nvision::vision
