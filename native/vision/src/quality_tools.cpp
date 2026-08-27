#include "quality_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/measurement.hpp"

#include <opencv2/imgproc.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nvision::vision {

namespace {

using acquisition::ImageView;
using acquisition::PixelFormat;
using core::Measurement;
using core::MeasurementKey;
using core::MeasurementList;
using core::MeasurementUnit;
using core::PortKey;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

bool is_intensity_image(const PixelFormat format) noexcept {
    return format == PixelFormat::mono8 || format == PixelFormat::mono16;
}

core::ExecutionError needs_intensity(const std::string& tool) {
    return configuration_error(
        "UNSUPPORTED_PIXEL_FORMAT",
        tool + " requires a single-channel intensity image; convert colour first");
}

// A quality figure describes the image as a whole, so it is a pure number rather than a
// length, and it names no coordinate space.
ToolExecutionOutcome single_measurement(
    const std::string& key,
    const double value,
    const MeasurementUnit unit) {
    PortValueMap outputs;
    outputs.set(
        PortKey("measurement"),
        PortValue::measurement(Measurement(MeasurementKey(key), value, unit)));
    return ToolExecutionOutcome::succeeded(std::move(outputs));
}

// Two rasters can only be compared where they describe the same extent.
std::optional<core::ExecutionError> comparable(const ImageView& left, const ImageView& right) {
    if (!is_intensity_image(left.pixel_format()) || !is_intensity_image(right.pixel_format())) {
        return needs_intensity("comparing two images");
    }
    if (left.pixel_format() != right.pixel_format()) {
        return configuration_error(
            "PIXEL_FORMAT_MISMATCH", "the two images do not share a pixel format");
    }
    if (left.width() != right.width() || left.height() != right.height()) {
        return configuration_error(
            "SIZE_MISMATCH", "the two images differ in size, so they cannot be compared");
    }
    return std::nullopt;
}

class HistogramTool final : public VisionTool {
public:
    HistogramTool() : VisionTool("nvision.tool.measurement.histogram", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (!is_intensity_image(image.pixel_format())) {
            // Which channel a colour histogram describes is a decision the graph makes,
            // so a channel is extracted before this tool rather than guessed inside it.
            return ToolExecutionOutcome::failed(needs_intensity("a histogram"));
        }

        const auto bins =
            static_cast<int>(context.parameter(core::ParameterKey("bins")).integer_value());
        const float upper = image.pixel_format() == PixelFormat::mono8 ? 256.0F : 65536.0F;
        const std::array<float, 2> range{0.0F, upper};
        const float* ranges = range.data();
        const std::array<int, 1> channels{0};

        const cv::Mat source = interop::borrow(image);
        cv::Mat counts;
        cv::calcHist(&source, 1, channels.data(), cv::Mat(), counts, 1, &bins, &ranges);

        MeasurementList histogram;
        histogram.reserve(static_cast<std::size_t>(bins));
        for (int bin = 0; bin < bins; ++bin) {
            // A bin holds how many pixels fell into it, which is a count and not a length.
            histogram.emplace_back(
                MeasurementKey("histogram.bin." + std::to_string(bin)),
                static_cast<double>(counts.at<float>(bin)),
                MeasurementUnit::count);
        }

        PortValueMap outputs;
        outputs.set(PortKey("histogram"), PortValue::measurement_list(std::move(histogram)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class SharpnessTool final : public VisionTool {
public:
    SharpnessTool() : VisionTool("nvision.tool.measurement.sharpness", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (!is_intensity_image(image.pixel_format())) {
            return ToolExecutionOutcome::failed(needs_intensity("sharpness"));
        }

        const std::string method =
            context.parameter(core::ParameterKey("method")).enumeration_value().value();
        const cv::Mat source = interop::borrow(image);

        double value = 0.0;
        if (method == "tenengrad") {
            // The mean squared gradient magnitude: a focused image has stronger edges.
            cv::Mat horizontal;
            cv::Mat vertical;
            cv::Sobel(source, horizontal, CV_64F, 1, 0);
            cv::Sobel(source, vertical, CV_64F, 0, 1);
            cv::Mat magnitude;
            cv::magnitude(horizontal, vertical, magnitude);
            value = cv::mean(magnitude.mul(magnitude))[0];
        } else {
            // The variance of the Laplacian: a blurred image has little second derivative.
            cv::Mat laplacian;
            cv::Laplacian(source, laplacian, CV_64F);
            cv::Scalar mean;
            cv::Scalar deviation;
            cv::meanStdDev(laplacian, mean, deviation);
            value = deviation[0] * deviation[0];
        }

        return single_measurement("sharpness", value, MeasurementUnit::none);
    }
};

class ContrastTool final : public VisionTool {
public:
    ContrastTool() : VisionTool("nvision.tool.measurement.contrast", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (!is_intensity_image(image.pixel_format())) {
            return ToolExecutionOutcome::failed(needs_intensity("contrast"));
        }

        // Root-mean-square contrast: the spread of intensity around its own mean. It is
        // defined for every image, unlike ratios that divide by the extremes.
        cv::Scalar mean;
        cv::Scalar deviation;
        cv::meanStdDev(interop::borrow(image), mean, deviation);
        return single_measurement("contrast", deviation[0], MeasurementUnit::none);
    }
};

class EdgeDensityTool final : public VisionTool {
public:
    EdgeDensityTool()
        : VisionTool("nvision.tool.measurement.edge-density", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        if (image.pixel_format() != PixelFormat::mono8) {
            return ToolExecutionOutcome::failed(configuration_error(
                "UNSUPPORTED_PIXEL_FORMAT",
                "edge density requires an 8-bit single-channel image"));
        }

        const double low =
            context.parameter(core::ParameterKey("low-threshold")).floating_point_value();
        const double high =
            context.parameter(core::ParameterKey("high-threshold")).floating_point_value();
        if (low > high) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_RANGE", "the edge low threshold is above its high threshold"));
        }

        cv::Mat edges;
        cv::Canny(interop::borrow(image), edges, low, high);
        const double density = static_cast<double>(cv::countNonZero(edges)) /
                               (static_cast<double>(image.width()) *
                                static_cast<double>(image.height()));
        return single_measurement("edge-density", density, MeasurementUnit::ratio);
    }
};

class NccTool final : public VisionTool {
public:
    NccTool() : VisionTool("nvision.tool.measurement.ncc", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const ImageView& reference = context.input(PortKey("reference")).image_value();
        if (const auto refusal = comparable(image, reference)) {
            return ToolExecutionOutcome::failed(*refusal);
        }

        // Correlating equal-sized rasters yields a single coefficient in [-1, 1].
        cv::Mat score;
        cv::matchTemplate(
            interop::borrow(image), interop::borrow(reference), score, cv::TM_CCOEFF_NORMED);

        PortValueMap outputs;
        outputs.set(
            PortKey("score"), PortValue::floating_point(static_cast<double>(score.at<float>(0, 0))));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

class SsimTool final : public VisionTool {
public:
    SsimTool() : VisionTool("nvision.tool.measurement.ssim", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const ImageView& image = context.input(PortKey("image")).image_value();
        const ImageView& reference = context.input(PortKey("reference")).image_value();
        if (const auto refusal = comparable(image, reference)) {
            return ToolExecutionOutcome::failed(*refusal);
        }

        // Structural similarity as defined by Wang et al.: local means, variances and
        // covariance under a Gaussian window, stabilised by the two standard constants.
        const double dynamic_range =
            image.pixel_format() == PixelFormat::mono8 ? 255.0 : 65535.0;
        const double c1 = (0.01 * dynamic_range) * (0.01 * dynamic_range);
        const double c2 = (0.03 * dynamic_range) * (0.03 * dynamic_range);

        cv::Mat first;
        cv::Mat second;
        interop::borrow(image).convertTo(first, CV_64F);
        interop::borrow(reference).convertTo(second, CV_64F);

        const cv::Size window(11, 11);
        const double sigma = 1.5;
        cv::Mat first_mean;
        cv::Mat second_mean;
        cv::GaussianBlur(first, first_mean, window, sigma);
        cv::GaussianBlur(second, second_mean, window, sigma);

        const cv::Mat first_mean_squared = first_mean.mul(first_mean);
        const cv::Mat second_mean_squared = second_mean.mul(second_mean);
        const cv::Mat cross_mean = first_mean.mul(second_mean);

        cv::Mat first_variance;
        cv::Mat second_variance;
        cv::Mat covariance;
        cv::GaussianBlur(first.mul(first), first_variance, window, sigma);
        cv::GaussianBlur(second.mul(second), second_variance, window, sigma);
        cv::GaussianBlur(first.mul(second), covariance, window, sigma);
        first_variance -= first_mean_squared;
        second_variance -= second_mean_squared;
        covariance -= cross_mean;

        const cv::Mat numerator = (2.0 * cross_mean + c1).mul(2.0 * covariance + c2);
        const cv::Mat denominator =
            (first_mean_squared + second_mean_squared + c1)
                .mul(first_variance + second_variance + c2);
        cv::Mat similarity;
        cv::divide(numerator, denominator, similarity);

        PortValueMap outputs;
        outputs.set(PortKey("score"), PortValue::floating_point(cv::mean(similarity)[0]));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_histogram_tool() {
    return std::make_shared<const HistogramTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_sharpness_tool() {
    return std::make_shared<const SharpnessTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_contrast_tool() {
    return std::make_shared<const ContrastTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_edge_density_tool() {
    return std::make_shared<const EdgeDensityTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_ncc_tool() {
    return std::make_shared<const NccTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_ssim_tool() {
    return std::make_shared<const SsimTool>();
}

}  // namespace nvision::vision
