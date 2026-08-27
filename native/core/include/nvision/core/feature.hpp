#pragma once

#include "nvision/core/geometry.hpp"
#include "nvision/core/identifier.hpp"

#include <cstddef>
#include <vector>

namespace nvision::core {

// One distinctive image location a detector found. The descriptor itself stays inside the
// backend that produced it: a workflow matches features, it does not read their bytes.
class Feature final {
public:
    Feature(Point2D position, double size, double angle_degrees, double response);

    [[nodiscard]] const Point2D& position() const noexcept;
    [[nodiscard]] double size() const noexcept;
    [[nodiscard]] double angle_degrees() const noexcept;
    [[nodiscard]] double response() const noexcept;

private:
    Point2D position_;
    double size_;
    double angle_degrees_;
    double response_;
};

// Features found in one raster, all in that raster's coordinate space.
class FeatureSet final {
public:
    FeatureSet(CoordinateSpaceId coordinate_space, std::vector<Feature> features);

    [[nodiscard]] const CoordinateSpaceId& coordinate_space() const noexcept;
    [[nodiscard]] const std::vector<Feature>& features() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    CoordinateSpaceId coordinate_space_;
    std::vector<Feature> features_;
};

// One accepted correspondence between a queried feature and a reference feature.
class FeatureMatch final {
public:
    FeatureMatch(std::size_t query_index, std::size_t reference_index, double distance);

    [[nodiscard]] std::size_t query_index() const noexcept;
    [[nodiscard]] std::size_t reference_index() const noexcept;
    // Descriptor distance. Smaller is a closer match; its scale depends on the backend.
    [[nodiscard]] double distance() const noexcept;

private:
    std::size_t query_index_;
    std::size_t reference_index_;
    double distance_;
};

// Correspondences together with both feature sets they index.
//
// Carrying the sets rather than only the index pairs is what lets a later node, such as a
// homography estimate, recover the actual point pairs without being handed the images
// again.
class MatchSet final {
public:
    MatchSet(FeatureSet query, FeatureSet reference, std::vector<FeatureMatch> matches);

    [[nodiscard]] const FeatureSet& query() const noexcept;
    [[nodiscard]] const FeatureSet& reference() const noexcept;
    [[nodiscard]] const std::vector<FeatureMatch>& matches() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    // The matched point pair, resolved through the sets this match set carries.
    [[nodiscard]] const Point2D& query_point(std::size_t match_index) const;
    [[nodiscard]] const Point2D& reference_point(std::size_t match_index) const;

private:
    FeatureSet query_;
    FeatureSet reference_;
    std::vector<FeatureMatch> matches_;
};

}  // namespace nvision::core
