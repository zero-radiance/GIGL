#include "PhotonTracer.h"
#include "RTBase.h"
#include "..\Common\Constants.h"
#include "..\Common\Random.h"
#include "..\Common\Scene.h"
#include "..\Common\Utility.hpp"
#include "..\VPL\PointLight.hpp"
#include "..\VPL\LightArray.hpp"

using glm::vec3;
using glm::mat3;
using glm::dot;
using glm::min;
using glm::max;

namespace rt {
    // This function generates a random direction on the unit hemisphere.
    // Distribution is uniform.
    // The direction is transformed in the provided coordinate system and returned together
    // with the probability with which the vector was generated.
    static inline void generateUniformDirection(const float u0, const float u1,
                                                vec3& dir, float& pdf) {
        // phi = 2 * PI * u0;
        // u1 = cos(the)
        // the = arccos(u1)
        // sin(the) = sqrt(1 - cos^2(the)) = sqrt(1 - u1^2)
        const float  phi{TWO_PI * u0};
        const float& cos_the{u1};
        const float  sin_the{sqrt(1.0f - cos_the * cos_the)};
        dir = genVecSinCos(phi, sin_the, cos_the);
        // Uniform PDF with respect to the solid angle, p = 1 / (2 * PI)
        pdf = 0.5f * INV_PI;
    }

    // Henyey-Greenstein phase function
    namespace HGPhaseFunc {
        static inline float eval(const float cos_the) {
            const float base{1.0f + sq(HG_G) - 2.0f * HG_G * cos_the};
            return 0.25f * INV_PI * (1.0f - sq(HG_G)) / (base * sqrt(base));
        }

        // Returns cos(the)
        static inline float sample(const float u) {
            const float n{1.0f - sq(HG_G)};
            const float d{1.0f - HG_G + 2.0f * HG_G * u};
            return 0.5f / abs(HG_G) * (1.0f + sq(HG_G) - sq(n / d));
        }

        static inline void importanceSample(const vec3& I, vec3& O, float& throughput) {
            const float phi{TWO_PI * UnitRNG::generate()};
            const float cos_the{HGPhaseFunc::sample(UnitRNG::generate())};
            const float sin_the{sqrt(1.0f - cos_the * cos_the)};
            const mat3 micro_to_macro{spanTangentSpace(I)};
            const vec3 micro_O{genVecSinCos(phi, sin_the, cos_the)};
            O = micro_to_macro * micro_O;
            // Function value is equal to its PDF
            throughput = 1.0f;
        }
    }

    // Calculates distance to next event within medium using Woodcock tracking algorithm
    static inline bool calcEventDistWT(const Scene& scene, const rt::Ray& ray,
                                       float& d_event, float& p_event) {
        const float maj_ext_k{scene.getMajExtK()};
        if (0.0f == maj_ext_k) { return false; }
        d_event = ray.t_min;
        p_event = 0.0f;
        while (d_event < ray.t_max && p_event < UnitRNG::generate()) {
            const float dt{-log(1.0f - UnitRNG::generate()) / maj_ext_k};
            d_event += dt;
            const vec3  s_pos{ray.getPtAtDist(d_event)};
            const float ext_k{scene.sampleExtK(s_pos)};
            p_event = ext_k / maj_ext_k;
        }
        return d_event < ray.t_max;
    }

    void PhotonTracer::trace(const Scene& scene, const PPL& source, const glm::vec3& shoot_dir,
                             const int max_vpl_count, LightArray<VPL>& la) {
        la.clear();
        uint n_paths{0};    // Total number of paths traced
        do {
            // Start a new path
            ++n_paths;
            // Switch between hemisphere and full sphere sampling
            const vec3 up{(source.wPos().z <= MAX_FOG_HEIGHT && UnitRNG::generate() < 0.5f) ?
                          -shoot_dir : shoot_dir};
            const mat3 microToMacro{spanTangentSpace(up)};
            vec3 dir; float pdf;
            generateUniformDirection(UnitRNG::generate(), UnitRNG::generate(), dir, pdf);
            // Compute initial ray parameters
            Ray  ray{source.wPos(), dir};
            vec3 attenuation{1.0f / pdf};
            for (int n_bounces = 1; n_bounces <= N_GI_BOUNCES; ++n_bounces) {
                // Trace ray through scene and fog
                const bool hit     = scene.trace(ray);
                const auto fog_hit = scene.traceFog(ray);
                // Volume scattering event?
                if (fog_hit) {
                    // Step through volume
                    ray.setValidRange(max(fog_hit.entr, 0.0f),
                                      min(fog_hit.exit, ray.inters.distance));
                    float t_event, p_event;
                    if (calcEventDistWT(scene, ray, t_event, p_event)) {
                        // Volume (scattering or absorption) event
                        const vec3  event_pt{ray.getPtAtDist(t_event)};
                        const float sca_k{scene.sampleScaK(event_pt)};
                        const float sca_albedo{scene.getScaAlbedo()};
                        // Russian roulette
                        if (UnitRNG::generate() < sca_albedo) {
                            // Scattering event; create a VPL
                            const vec3 I{ray.d};
                            const vec3 vpl_intens{source.intensity() * attenuation};
                            la.addLight(VPL{n_paths - 1, event_pt, I, vpl_intens, sca_k});
                            if (la.size() == max_vpl_count) break;
                            // Perform Importance Sampling of Henyey-Greenstein phase function
                            vec3 O; float throughput;
                            HGPhaseFunc::importanceSample(I, O, throughput);
                            // Increase attenuation using volume rendering equation
                            // WT implicitly accounts for extinction
                            // RR implicitly accounts for albedo
                            attenuation *= throughput;
                            // Generate new direction
                            ray = Ray{event_pt, O};
                            // Trace a new ray
                            continue;
                        } else {
                            // Absorption event due to Russian roulette; terminate path
                            break;
                        }
                    } else {
                        // No volume scattering event; check for surface reflection (below)
                    }
                }
                // Surface reflection event?
                if (hit) {
                    const vec3  hit_pt{ray.getPtAtDist(ray.inters.distance)};
                    const vec3& norm{ray.inters.normal};
                    if (dot(ray.d, ray.inters.normal) >= 0.0f || hit_pt.z > MAX_FOG_HEIGHT) {
                        // Invalid intersection
                        break;
                    }
                    const PhongMaterial* mat{ray.inters.material};
                    if (mat->isEmissive()) {
                        // Avoid energy overestimation
                        break;
                    } else {
                        // Continue tracing the path
                        // Russian roulette
                        if (UnitRNG::generate() < SURVIVAL_P_RR) {
                            // Photon survives
                            attenuation /= SURVIVAL_P_RR;
                            // Create PL
                            const vec3 I{-ray.d};
                            // Resulting intensity is component-wise multiplication
                            const vec3 vpl_intens{source.intensity() * attenuation};
                            la.addLight(VPL{n_paths - 1, hit_pt, I, vpl_intens, norm,
                                          mat->kD(), mat->kS(), mat->nS()});
                            if (la.size() == max_vpl_count) break;
                            // Perform Importance Sampling of Phong BRDF
                            vec3 O, throughput;
                            mat->importanceSampleBRDF(I, norm, O, throughput);
                            // Increase attenuation using rendering equation
                            const float cos_the{dot(O, norm)};
                            attenuation *= throughput * cos_the;
                            // Raise ray origin slightly above surface
                            ray = Ray{hit_pt, O, norm};
                            // Trace a new ray
                            continue;
                        } else {
                            // Absorption event due to Russian roulette; terminate path
                            break;
                        }
                    }
                } else {
                    // No events; terminate path
                    break;
                }
            }
        } while (la.size() < max_vpl_count && n_paths < static_cast<uint>(100 * max_vpl_count));
        if (la.size() < max_vpl_count) {
            // Unable to (efficiently) create VPLs; abort
            la.clear();
            printError("VPL tracing problems.");
        } else {
            la.normalizeIntensity(n_paths);
        }
    }
}
