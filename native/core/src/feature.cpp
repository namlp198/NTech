#include "nvision/core/feature.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nvision::core {

Feature::Feature(
    Point2D position,
    const double size,
    const double angle_degrees,
    const double response)
    : position_(std::move(position)),
      size_(size),
      angle_degrees_(angle_degrees),
      response_(response) {
    if (!std::isfinite(size_) || size_ <= 0.0) {
        throw std::invalid_argument("a feature size must be finite and positive");
    }
    if (!std::isfinite(angle_degrees_)) {
        throw std::invalid_argument("a feature angle must be finite");
    }
    if (!std::isfinite(response_)) {
        throw std::invalid_argument("a feature response must be finite");
    }
}

const Point2D& Feature::position() const noexcept {
    return position_;
}

double Feature::size() const noexcept {
    return size_;
}

double Feature::angle_degrees() const noexcept {
    return angle_degrees_;
}

double Feature::response() const noexcept {
    return response_;
}

FeatureSet::FeatureSet(CoordinateSpaceId coordinate_space, std::vector<Feature> features)
    : coordinate_space_(std::move(coordinate_space)), features_(std::move(features)) {
    if (!std::all_of(features_.begin(), features_.end(), [this](const Feature& feature) {
            return feature.position().coordinate_space() == coordinate_space_;
        })) {
        throw std::invalid_argument(
            "every feature must be in the coordinate space of the set that holds it");
    }
}

const CoordinateSpaceId& FeatureSet::coordinate_space() const noexcept {
    return coordinate_space_;
}

const std::vector<Feature>& FeatureSet::features() const noexcept {
    return features_;
}

std::size_t FeatureSet::size() const noexcept {
    return features_.size();
}

FeatureMatch::FeatureMatch(
    const std::size_t query_index,
    const std::size_t reference_index,
    const double distance)
    : query_index_(query_index), reference_index_(reference_index), distance_(distance) {
    if (!std::isfinite(distance_) || distance_ < 0.0) {
        throw std::invalid_argument("a match distance must be finite and non-negative");
    }
}

std::size_t FeatureMatch::query_index() const noexcept {
    return query_index_;
}

std::size_t FeatureMatch::reference_index() const noexcept {
    return reference_index_;
}

double FeatureMatch::distance() const noexcept {
    return distance_;
}

MatchSet::MatchSet(FeatureSet query, FeatureSet reference, std::vector<FeatureMatch> matches)
    : query_(std::move(query)), reference_(std::move(reference)), matches_(std::move(matches)) {
    if (query_.coordinate_space() == reference_.coordinate_space()) {
        // Matching a raster against itself has no meaning for alignment, and letting it
        // through would produce an identity homography that looks like a real result.
        throw std::invalid_argument(
            "a match set must relate two different coordinate spaces");
    }
    for (const FeatureMatch& match : matches_) {
        if (match.query_index() >= query_.size() ||
            match.reference_index() >= reference_.size()) {
            throw std::invalid_argument("a match index is outside its feature set");
        }
    }
}

const FeatureSet& MatchSet::query() const noexcept {
    return query_;
}

const FeatureSet& MatchSet::reference() const noexcept {
    return reference_;
}

const std::vector<FeatureMatch>& MatchSet::matches() const noexcept {
    return matches_;
}

std::size_t MatchSet::size() const noexcept {
    return matches_.size();
}

const Point2D& MatchSet::query_point(const std::size_t match_index) const {
    if (match_index >= matches_.size()) {
        throw std::out_of_range("match index is outside the match set");
    }
    return query_.features()[matches_[match_index].query_index()].position();
}

const Point2D& MatchSet::reference_point(const std::size_t match_index) const {
    if (match_index >= matches_.size()) {
        throw std::out_of_range("match index is outside the match set");
    }
    return reference_.features()[matches_[match_index].reference_index()].position();
}

}  // namespace nvision::core
