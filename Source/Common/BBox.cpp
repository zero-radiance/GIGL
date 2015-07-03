#include "BBox.h"
#include <GLM\common.hpp>
#include "..\RT\RTBase.h"

using glm::vec3;
using glm::min;
using glm::max;

BBox::IntDist::IntDist(): entr{FLT_MAX}, exit{-FLT_MAX} {}

BBox::IntDist::IntDist(const float t_entr, const float t_exit): entr{t_entr}, exit{t_exit} {}

BBox::IntDist::operator bool() const {
    return entr < FLT_MAX;
}

BBox::BBox() {
    m_bound_pts[0] = vec3{ FLT_MAX};
    m_bound_pts[1] = vec3{-FLT_MAX};
}

BBox::BBox(const vec3& min_pt, const vec3& max_pt) {
    m_bound_pts[0] = min_pt;
    m_bound_pts[1] = max_pt;
}

BBox BBox::empty() {
    return BBox{};
}

void BBox::extend(const vec3& p) {
    for (auto axis = 0; axis < 3; ++axis) {
        m_bound_pts[0][axis] = min(m_bound_pts[0][axis], p[axis]);
        m_bound_pts[1][axis] = max(m_bound_pts[1][axis], p[axis]);
    }
}

void BBox::extend(const BBox& bbox) {
    extend(bbox.m_bound_pts[0]);
    extend(bbox.m_bound_pts[1]);
}

bool BBox::containsPoint(const vec3& p) const {
    return m_bound_pts[0].x <= p.x && p.x <= m_bound_pts[1].x &&
           m_bound_pts[0].y <= p.y && p.y <= m_bound_pts[1].y &&
           m_bound_pts[0].z <= p.z && p.z <= m_bound_pts[1].z;
}

vec3 BBox::dimensions() const {
    return m_bound_pts[1] - m_bound_pts[0];
}

vec3 BBox::computeNormPos(const vec3& w_pos) const {
    return (w_pos - minPt()) / dimensions();
}

float BBox::computeArea() const {
    const vec3 dims{dimensions()};
    return 2.0f * (dims.x * dims.y + (dims.x + dims.y) * dims.z);
}

int BBox::maxExtentAxis() const {
    const vec3 diag{m_bound_pts[1] - m_bound_pts[0]};
    if (diag.x > diag.y && diag.x > diag.z) {
        return 0;
    } else if (diag.y > diag.z) {
        return 1;
    } else {
        return 2;
    }
}

const vec3& BBox::minPt() const {
    return m_bound_pts[0];
}

const vec3& BBox::maxPt() const {
    return m_bound_pts[1];
}

BBox::IntDist BBox::intersect(const rt::Ray& ray) const {
    float t_entr, t_exit, t_y_min, t_y_max, t_z_min, t_z_max;

    const bool sign[3] = {ray.inv_d.x >= 0.0f, ray.inv_d.y >= 0.0f, ray.inv_d.z >= 0.0f};

    t_entr  = (m_bound_pts[1 - sign[0]].x - ray.o.x) * ray.inv_d.x;
    t_exit  = (m_bound_pts[    sign[0]].x - ray.o.x) * ray.inv_d.x;
    t_y_min = (m_bound_pts[1 - sign[1]].y - ray.o.y) * ray.inv_d.y;
    t_y_max = (m_bound_pts[    sign[1]].y - ray.o.y) * ray.inv_d.y;

    if ((t_entr > t_y_max) || (t_y_min > t_exit)) { return BBox::IntDist{}; }

    t_entr  = max(t_entr, t_y_min);
    t_exit  = min(t_exit, t_y_max);

    t_z_min = (m_bound_pts[1 - sign[2]].z - ray.o.z) * ray.inv_d.z;
    t_z_max = (m_bound_pts[    sign[2]].z - ray.o.z) * ray.inv_d.z;

    if ((t_entr > t_z_max) || (t_z_min > t_exit)) { return BBox::IntDist{}; }

    t_entr = max(t_entr, t_z_min);
    t_exit = min(t_exit, t_z_max);

    if (t_entr < ray.t_max && t_exit > ray.t_min) {
        return BBox::IntDist{t_entr, t_exit};
    } else {
        return BBox::IntDist{};
    }
}
