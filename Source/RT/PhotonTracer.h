#pragma once

#include <GLM\fwd.hpp>

class PPL;
class VPL;
class Scene;
template <class PL> class LightArray;

/* Photon (particle) tracer which fills an array with VPLs */
namespace rt {
    namespace PhotonTracer {
        void trace(const Scene& scene, const PPL& source, const glm::vec3& shoot_dir,
                   const int max_vpl_count, LightArray<VPL>& la);
    };
}
