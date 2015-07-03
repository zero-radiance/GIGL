#pragma once

#include <GLM\vec3.hpp>
#include "Definitions.h"

namespace rt { class Ray; }

/* Axis-aligned bounding box (AABB) */
class BBox {
public:
    BBox();
    RULE_OF_ZERO(BBox);
    // Constructor using specified bounds
    explicit BBox(const glm::vec3& min_pt, const glm::vec3& max_pt);
    // Returns an "empty" bounding box
    static BBox empty();
    // Extend bounding box to contain a point
    void extend(const glm::vec3& p);
    // Extend the bounding box to contain another bounding box
    void extend(const BBox& bbox);
    // Check whether the point is located inside bounding box
    bool containsPoint(const glm::vec3& p) const;
    // Returns dimensions of bounding box
    glm::vec3 dimensions() const;
    // Computes normalized position within BBox from specified absolute position
    glm::vec3 computeNormPos(const glm::vec3& w_pos) const;
    // Computes surface area of bounding box
    float computeArea() const;
    // Returns axis of maximal extent
    int maxExtentAxis() const;
    // Returns the bottom near left bounding point 
    const glm::vec3& minPt() const;
    // Returns the top far right bounding point 
    const glm::vec3& maxPt() const;
    // Distances to entry and exit points of intersection with BBox
    // If there is no intersection, then the entry distance is FLT_MAX
    struct IntDist {
        IntDist();
        explicit IntDist(const float t_entr, const float t_exit);
        operator bool() const;
        // Storage
        float entr, exit;
    };
    // Williams et al. intersection algorithm (2005)
    // Returns entry and exit distances of ray with bounding box
    BBox::IntDist intersect(const rt::Ray& ray) const;
private:
    glm::vec3 m_bound_pts[2];     // two points - min and max bounds
};
