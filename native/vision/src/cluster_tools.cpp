#include "cluster_tools.hpp"

#include "tool_support.hpp"

#include "nvision/core/measurement.hpp"

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace nvision::vision {

namespace {

using core::CoordinateSpaceId;
using core::Measurement;
using core::MeasurementKey;
using core::MeasurementList;
using core::MeasurementUnit;
using core::ParameterKey;
using core::Point2D;
using core::PortKey;
using workflow::PortValue;
using workflow::PortValueMap;
using workflow::ToolExecutionContext;
using workflow::ToolExecutionOutcome;

// A cluster label is an index into the clusters a run found, not a length and not a
// tally, so it is a pure number. Noise carries -1, which only `none` admits.
MeasurementList labels_of(const std::vector<int>& labels) {
    MeasurementList measurements;
    measurements.reserve(labels.size());
    for (std::size_t index = 0U; index < labels.size(); ++index) {
        measurements.emplace_back(
            MeasurementKey("point." + std::to_string(index) + ".cluster"),
            static_cast<double>(labels[index]),
            MeasurementUnit::none);
    }
    return measurements;
}

double separation(const Point2D& left, const Point2D& right) noexcept {
    return std::hypot(left.x() - right.x(), left.y() - right.y());
}

// The centre of each cluster, in cluster-label order, skipping labels nothing landed in.
std::vector<Point2D> centres_of(
    const CoordinateSpaceId& space,
    const std::vector<Point2D>& points,
    const std::vector<int>& labels,
    const int cluster_count) {
    std::vector<Point2D> centres;
    for (int cluster = 0; cluster < cluster_count; ++cluster) {
        double x = 0.0;
        double y = 0.0;
        std::size_t members = 0U;
        for (std::size_t index = 0U; index < points.size(); ++index) {
            if (labels[index] != cluster) {
                continue;
            }
            x += points[index].x();
            y += points[index].y();
            ++members;
        }
        if (members == 0U) {
            continue;
        }
        centres.emplace_back(
            space, x / static_cast<double>(members), y / static_cast<double>(members));
    }
    return centres;
}

ToolExecutionOutcome clustered(
    const CoordinateSpaceId& space,
    const std::vector<Point2D>& points,
    const std::vector<int>& labels,
    const int cluster_count) {
    PortValueMap outputs;
    outputs.set(PortKey("labels"), PortValue::measurement_list(labels_of(labels)));
    outputs.set(
        PortKey("centers"),
        PortValue::point_list(centres_of(space, points, labels, cluster_count)));
    return ToolExecutionOutcome::succeeded(std::move(outputs));
}

class KMeansTool final : public VisionTool {
public:
    KMeansTool() : VisionTool("nvision.tool.structure.k-means", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        const auto clusters =
            static_cast<int>(context.parameter(ParameterKey("cluster-count")).integer_value());
        const auto iterations =
            static_cast<int>(context.parameter(ParameterKey("iterations")).integer_value());

        if (points.empty()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS", "clustering needs at least one point"));
        }
        if (static_cast<std::size_t>(clusters) > points.size()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS",
                "the requested cluster count exceeds the number of points supplied"));
        }

        cv::Mat samples(static_cast<int>(points.size()), 2, CV_32F);
        for (std::size_t index = 0U; index < points.size(); ++index) {
            samples.at<float>(static_cast<int>(index), 0) = static_cast<float>(points[index].x());
            samples.at<float>(static_cast<int>(index), 1) = static_cast<float>(points[index].y());
        }

        // K-Means seeds its centres from OpenCV's thread-local generator. Fixing that
        // generator is what makes a repeated inspection produce the same clustering, which
        // the platform requires of every tool.
        cv::theRNG() = cv::RNG(deterministic_seed);

        cv::Mat assigned;
        cv::kmeans(
            samples,
            clusters,
            assigned,
            cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, iterations, 1.0),
            1,
            cv::KMEANS_PP_CENTERS);

        std::vector<int> labels(points.size());
        for (std::size_t index = 0U; index < points.size(); ++index) {
            labels[index] = assigned.at<int>(static_cast<int>(index));
        }
        return clustered(points.front().coordinate_space(), points, labels, clusters);
    }

private:
    static constexpr std::uint64_t deterministic_seed = 0x4E56'4953'494F'4E01ULL;
};

class DbscanTool final : public VisionTool {
public:
    DbscanTool() : VisionTool("nvision.tool.structure.dbscan", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        if (points.empty()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS", "clustering needs at least one point"));
        }

        const double epsilon =
            context.parameter(ParameterKey("epsilon")).floating_point_value();
        const auto minimum_samples = static_cast<std::size_t>(
            context.parameter(ParameterKey("minimum-samples")).integer_value());

        // Density clustering, expanded in input order so a repeated run labels the same
        // points the same way. A point that never joins a dense region stays noise at -1.
        constexpr int unvisited = -2;
        constexpr int noise = -1;
        std::vector<int> labels(points.size(), unvisited);
        int next_cluster = 0;

        for (std::size_t seed = 0U; seed < points.size(); ++seed) {
            if (labels[seed] != unvisited) {
                continue;
            }
            std::vector<std::size_t> neighbours = within(points, seed, epsilon);
            if (neighbours.size() < minimum_samples) {
                labels[seed] = noise;
                continue;
            }

            const int cluster = next_cluster++;
            labels[seed] = cluster;
            for (std::size_t cursor = 0U; cursor < neighbours.size(); ++cursor) {
                const std::size_t member = neighbours[cursor];
                if (labels[member] == noise) {
                    // A former noise point on the edge of a dense region joins it, but
                    // does not extend it further.
                    labels[member] = cluster;
                }
                if (labels[member] != unvisited) {
                    continue;
                }
                labels[member] = cluster;
                const std::vector<std::size_t> reachable = within(points, member, epsilon);
                if (reachable.size() >= minimum_samples) {
                    neighbours.insert(neighbours.end(), reachable.begin(), reachable.end());
                }
            }
        }

        return clustered(points.front().coordinate_space(), points, labels, next_cluster);
    }

private:
    [[nodiscard]] static std::vector<std::size_t> within(
        const std::vector<Point2D>& points,
        const std::size_t centre,
        const double epsilon) {
        std::vector<std::size_t> neighbours;
        for (std::size_t index = 0U; index < points.size(); ++index) {
            if (separation(points[centre], points[index]) <= epsilon) {
                neighbours.push_back(index);
            }
        }
        return neighbours;
    }
};

class MeanShiftTool final : public VisionTool {
public:
    MeanShiftTool() : VisionTool("nvision.tool.structure.mean-shift", initial_tool_version()) {}

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        const auto& points = context.input(PortKey("points")).point_list_value();
        if (points.empty()) {
            return ToolExecutionOutcome::failed(configuration_error(
                "NOT_ENOUGH_POINTS", "mean shift needs at least one point"));
        }

        const double bandwidth =
            context.parameter(ParameterKey("bandwidth")).floating_point_value();
        if (bandwidth <= 0.0) {
            return ToolExecutionOutcome::failed(configuration_error(
                "INVALID_BANDWIDTH", "the mean shift bandwidth must be positive"));
        }

        // Every point walks uphill to the mean of its neighbourhood until it settles. The
        // number of modes is discovered rather than requested, which is the reason to
        // choose mean shift over k-means.
        const CoordinateSpaceId& space = points.front().coordinate_space();
        std::vector<Point2D> modes;
        for (const Point2D& start : points) {
            double x = start.x();
            double y = start.y();
            for (int step = 0; step < maximum_steps; ++step) {
                double sum_x = 0.0;
                double sum_y = 0.0;
                std::size_t members = 0U;
                for (const Point2D& candidate : points) {
                    if (std::hypot(candidate.x() - x, candidate.y() - y) > bandwidth) {
                        continue;
                    }
                    sum_x += candidate.x();
                    sum_y += candidate.y();
                    ++members;
                }
                if (members == 0U) {
                    break;
                }
                const double next_x = sum_x / static_cast<double>(members);
                const double next_y = sum_y / static_cast<double>(members);
                const double moved = std::hypot(next_x - x, next_y - y);
                x = next_x;
                y = next_y;
                if (moved <= convergence) {
                    break;
                }
            }

            // Modes that settled within half a bandwidth of each other are the same mode.
            const Point2D settled(space, x, y);
            const bool already_found = std::any_of(
                modes.begin(), modes.end(), [&settled, bandwidth](const Point2D& mode) {
                    return separation(mode, settled) <= bandwidth / 2.0;
                });
            if (!already_found) {
                modes.push_back(settled);
            }
        }

        PortValueMap outputs;
        outputs.set(PortKey("centers"), PortValue::point_list(std::move(modes)));
        return ToolExecutionOutcome::succeeded(std::move(outputs));
    }

private:
    static constexpr int maximum_steps = 100;
    static constexpr double convergence = 1.0e-6;
};

}  // namespace

std::shared_ptr<const workflow::ToolImplementation> create_k_means_tool() {
    return std::make_shared<const KMeansTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_dbscan_tool() {
    return std::make_shared<const DbscanTool>();
}

std::shared_ptr<const workflow::ToolImplementation> create_mean_shift_tool() {
    return std::make_shared<const MeanShiftTool>();
}

}  // namespace nvision::vision
