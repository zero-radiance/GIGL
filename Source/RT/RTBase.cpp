#include "RTBase.h"
#include "..\Common\Constants.h"
#include "..\Common\Utility.hpp"
#include "..\Common\Random.h"
#include "..\Common\Scene.h"

using glm::vec2;
using glm::vec3;
using glm::uvec3;
using glm::mat3;
using glm::min;
using glm::max;
using glm::dot;
using glm::cross;
using glm::length;
using glm::normalize;

extern Scene* scene;

namespace rt {
    // Reflects vector V around normal N; both vectors point away from the surface!
    static inline vec3 reflect(const vec3& V, const vec3& N) {
        return -V + 2.0f * dot(V, N) * N;
    }

    // Generates cosine-distributed random direction on the hemisphere
    static inline vec3 genCosRandHemiDir(const float u1, const float u2) {
        // phi        = 2 * PI * u1;
        // the        = acos(sqrt(u2))
        // cos^2(the) = sqrt(u2)
        // sin(the)   = sqrt(1 - cos^2(the)) = sqrt(1 - u2)
        const float  phi{TWO_PI * u1};
        const float& sq_cos_the{u2};
        const float  sin_the{sqrt(1.0f - sq_cos_the)};
        return genVecSinCos(phi, sin_the, sqrt(sq_cos_the));
    }

    // Generates cosine^n-distributed random direction on the hemisphere
    static inline vec3 genCosPowRandHemiDir(const float u1, const float u2, const float n) {
        // phi = 2 * PI * u1;
        // the = arccos(u2^(1 / (n + 1)))
        // cos(the) = u2^(1 / (n + 1))
        // sin(the) = sqrt(1 - cos^2(the))
        const float  phi{TWO_PI * u1};
        const float& cos_pow_the{u2};
        const float  cos_the{pow(cos_pow_the, 1.0f / (n + 1.0f))};
        const float  sin_the{sqrt(1.0f - cos_the * cos_the)};
        return genVecSinCos(phi, sin_the, cos_the);
    }

    PhongMaterial::PhongMaterial(const vec3& k_d, const vec3& k_s, const float n_s,
                                 const vec3& k_e): m_k_d(k_d), m_k_s(k_s), m_n_s(n_s), m_k_e(k_e) {}

    bool PhongMaterial::isEmissive() const {
        return m_k_e.r > 0.0f || m_k_e.g > 0.0f || m_k_e.b > 0.0f;
    }

    const glm::vec3& PhongMaterial::kD() const {
        return m_k_d;
    }

    const glm::vec3& PhongMaterial::kS() const {
        return m_k_s;
    }
    
    float PhongMaterial::nS() const {
        return m_n_s;
    }

    void PhongMaterial::importanceSampleBRDF(const vec3& I, const vec3& N, vec3& O,
                                             vec3& throughput) const {
        assert_normalization(I);
        assert_normalization(N);
        const float u1{UnitRNG::generate()}, u2{UnitRNG::generate()};
        // Using sRGB luminance weights for red, green, blue
        static const vec3 luminous_efficiency{0.2126f, 0.7152f, 0.0722f};
        const float diffuse_weight{dot(m_k_d, luminous_efficiency)};
        const float glossy_weight {dot(m_k_s, luminous_efficiency)};
        // Normalizing probabilities
        const float survival_p_diffuse{diffuse_weight / (diffuse_weight + glossy_weight)};
        // Roll a die
        const float u_rand{UnitRNG::generate()};
        if (u_rand < survival_p_diffuse) {
            /* Sampling diffuse component -only- */
            // Define transformation matrix from macroscopic to microscopic coordinate system
            const mat3 micro_to_macro{spanTangentSpace(N)};
            // Cosine-distributed hemisphere sampling
            const vec3 micro_O{genCosRandHemiDir(u1, u2)};
            O = micro_to_macro * micro_O;
            // Diffuse BRDF component value = k_d
            // Cosine ditribution, p = cos(the) / PI
            const float cos_the{micro_O.z};
            throughput = m_k_d * PI / (cos_the * survival_p_diffuse);
        } else {
            /* Sampling the glossy component -only- */
            const vec3 S{reflect(I, N)};
            // Define transformation matrix from macroscopic to microscopic coordinate system
            // Center micro-CS around the direction of specular reflection
            const mat3 micro_to_macro{spanTangentSpace(S)};
            // Cosine^n--distributed hemisphere sampling
            const vec3 micro_O{genCosPowRandHemiDir(u1, u2, m_n_s)};
            O = micro_to_macro * micro_O;
            // Cosine power ditribution, p = (n + 1) * cos^n(the) / (2 * PI)
            // The glossy part of Phong BRDF is k_s * cos^n(O, S)
            // Since we use S (and not an actual normal N), cos^n(the) = cos^n(O, S)
            const float survival_p_glossy{1.0f - survival_p_diffuse};
            throughput = m_k_s * TWO_PI / ((m_n_s + 1.0f) * survival_p_glossy);
        }
    }

    Ray::Ray(const vec3& origin, const vec3& direction): o{origin}, d{direction},
                                                         inv_d{1.0f / direction},
                                                         t_min{0.0f}, t_max{FLT_MAX} {
        assert_normalization(direction);
    }

    Ray::Ray(const vec3& origin, const vec3& direction,
             const vec3& normal): o{origin + RAY_OFFSET * normal}, d{direction},
                                  inv_d{1.0f / direction}, t_min{0.0f}, t_max{FLT_MAX} {
        assert_normalization(direction);
        assert_normalization(normal);
    }

    vec3 Ray::getPtAtDist(const float t) const {
        return o + t * d;
    }

    void Ray::setValidRange(const float min_dist, const float max_dist) {
        t_min = min_dist;
        t_max = max_dist;
    }

    Ray::Intersection::Intersection(): material{nullptr}, distance{FLT_MAX}, normal{0.0f} {}

    Triangle::Triangle(const uvec3& indices, const uint mat_id): m_indices{indices},
                                                                 m_mat_id{mat_id} {}

    const PhongMaterial* Triangle::material() const {
        return scene->getMaterial(m_mat_id);
    }

    BBox Triangle::computeBBox() const {
        BBox box{};
        for (uint i = 0; i < 3; ++i) {
            box.extend(scene->getVertex(m_indices[i]));
        }
        return box;
    }

    bool Triangle::intersect(Ray& ray) const {
        const vec3& pt0{scene->getVertex(m_indices[0])};
        const vec3& pt1{scene->getVertex(m_indices[1])};
        const vec3& pt2{scene->getVertex(m_indices[2])};
        const vec3  edge0{pt1 - pt0};
        const vec3  edge1{pt2 - pt0};
        const vec3  pVec{cross(ray.d, edge1)};
        const float det{dot(edge0, pVec)};
        if (det != 0.0f) {
            const float invDet{1.0f / det};
            const vec3  tVec{ray.o - pt0};
            const vec3  qVec{cross(tVec, edge0)};
            // Compute barycentric UV coords
            const float u{dot(tVec, pVec) * invDet};
            const float v{dot(ray.d, qVec) * invDet};
            if (u >= -TRI_EPS && v >= -TRI_EPS && (u + v) <= 1.0f + 2.0f * TRI_EPS) {
                const float dist{dot(edge1, qVec) * invDet};
                if (ray.t_min < dist && dist < ray.t_max && dist < ray.inters.distance) {
                    // Intersection found
                    ray.inters.distance = dist;
                    ray.inters.normal   = cross(edge0, edge1);
                    ray.inters.material = material();
                    return true;
                }
            }
        }
        return false;
    }
}
