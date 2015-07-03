#pragma once

#include <limits>
#include <GLM\mat3x3.hpp>
#include <GLM\geometric.hpp>
#include "..\Common\Definitions.h"

class BBox;

namespace rt {
    // Difference between 1.0f and the next <float>
    CONSTEXPR float EPS{std::numeric_limits<float>::epsilon()};

    // Assert whether input vector is normalized
    #define assert_normalization(V) assert(abs(glm::length(V) - 1.0f) <= 8.0f * EPS)

    // Returns EPS matching exponent of the data
    static inline float getRescaledEPS(const float t) {
        // FP representation: significand * (2 ^ exp)
        // Return 2 * EPS * (2 ^ exp) = EPS * (2 ^ (exp + 1))
        int exp;
        frexp(t, &exp);
        ++exp;
        if (exp >= 0) {
            return EPS * (1 << exp);
        } else {
            return EPS / (1 << -exp);
        }
    }

    // Span orthonormal right-handed tangent space around the normal and represent it as a matrix
    // Tangent is usually X, bitangent Y, normal Z
    static inline glm::mat3 spanTangentSpace(const glm::vec3& normal) {
        assert_normalization(normal);
        const glm::vec3 c1{glm::cross(normal, glm::vec3{0.0f, 0.0f, 1.0f})};
        const glm::vec3 c2{glm::cross(normal, glm::vec3{0.0f, 1.0f, 0.0f})};
        const glm::vec3 c3{glm::length(c1) > glm::length(c2) ? c1 : c2};
        const glm::vec3 tangent{glm::normalize(-c3)};  // Make the CS right-handed
        const glm::vec3 bitangent{glm::cross(normal, tangent)};
        return glm::mat3{tangent, bitangent, normal};
    }

    // Generates unit vector on the hemisphere
    static inline glm::vec3 genVecSinCos(const float phi, const float sin_the,
                                         const float cos_the) {
        // x = sin(the) * cos(phi)
        // y = sin(the) * sin(phi)
        // z = cos(the)
        return glm::vec3{sin_the * cos(phi), sin_the * sin(phi), cos_the};
    }

    /* Phong BRDF compatible material class */
    class PhongMaterial {
    public:
        PhongMaterial() = delete;
        RULE_OF_ZERO(PhongMaterial);
        // Constructs a material with coefficients for Phong BRDF
        explicit PhongMaterial(const glm::vec3& k_d, const glm::vec3& k_s,
                               const float n_s, const glm::vec3& k_e);
        // Returns true if material is emissive
        bool isEmissive() const;
        // Performs importance sampling of Phong BRDF
        void importanceSampleBRDF(const glm::vec3& I, const glm::vec3& N, glm::vec3& O,
                                  glm::vec3& throughput) const;
        // Returns the value of diffuse coefficient
        const glm::vec3& kD() const;
        // Returns the value of specular coefficient
        const glm::vec3& kS() const;
        // Returns the value of specular exponent
        float nS() const;
    private:
        glm::vec3 m_k_d;			// Diffuse coefficient
        glm::vec3 m_k_s;			// Specular coefficient
        float	  m_n_s;			// Specular exponent
        glm::vec3 m_k_e;	        // Emission [coefficient]
    };

    /* Ray class with origin o and direction d */
    class Ray {
    public:
        Ray() = delete;
        RULE_OF_ZERO(Ray);
        // Constructs a ray with specified origin and direction
        explicit Ray(const glm::vec3& origin, const glm::vec3& direction);
        // Constructor displacing ray origin along normal direction
        explicit Ray(const glm::vec3& origin, const glm::vec3& direction,
                     const glm::vec3& normal);
        // Returns a point along the ray at _distance away from origin
        glm::vec3 getPtAtDist(const float t) const;
        // Sets valid intersection range
        void setValidRange(const float min_dist, const float max_dist);
        // Public data members
        struct Intersection {
            Intersection();
            const PhongMaterial* material;
            float                distance;
            glm::vec3            normal;
        } inters;                   // Stores ray-primitive intersection information
        glm::vec3 o;				// Origin
        glm::vec3 d;				// Direction
        glm::vec3 inv_d;			// Inverse of direction
        float     t_min, t_max;     // Valid intersection distance range, s.t. t_max >= t_min > 0
    };

    /* Triangle primitive class */
    class Triangle {
    public:
        Triangle() = delete;
        RULE_OF_ZERO(Triangle);
        // Constructor, accepts indices of vert/norm/texcoords, and material index
        explicit Triangle(const glm::uvec3& indices, const uint mat_id);
        // Returns material associated with the triangle
        const PhongMaterial* material() const;
        // Computes BBox encompassing triangle
        BBox computeBBox() const;
        // Möller-Trumbore intersection algorithm (1997)
        // Returns true if ray intersects triangle, false otherwise
        bool intersect(Ray& ray) const;
    private:
        glm::uvec3 m_indices;       // Indices in vectors of vertices, normals and tex. coords
        uint       m_mat_id;		// Index in vector of materials
    };
}
