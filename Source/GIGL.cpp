#include <GLM\gtc\matrix_transform.hpp>
#include "Common\Constants.h"
#include "Common\Camera.h"
#include "Common\Random.h"
#include "Common\Scene.h"
#include "Common\Timer.h"
#include "Common\Utility.hpp"
#include "Common\Halton.hpp"
#include "RT\PhotonTracer.h"
#include "VPL\OmniShadowMap.hpp"
#include "VPL\PointLight.hpp"
#include "GL\GLShader.hpp"
#include "GL\GLTextureBuffer.h"
#include "GL\RTBLockMngr.h"
#include "GL\GLUniformManager.hpp"
#include "UI\InputHandler.h"
#include "UI\Window.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using glm::min;
using glm::normalize;

Scene* scene;

//*******************************
//*  World         X = right    *
//*  coordinate    Y = forward  *
//*  system		   Z = up       *
//*******************************

int main(int, char**) {
    // Randomize
    UnitRNG::init();
    // Create window
    Window window{WINDOW_RES, WINDOW_RES};
    if (!window.isOpen()) return -1;
    // Set up camera
    const PerspectiveCamera cam{{278.0f, 273.0f, -800.0f},  // Position
                                {0.0f, 0.0f, 1.0f},			// Direction
                                {0.0f, 1.0f, 0.0f},			// Up
                                0.22f * PI,					// Horizontal FoV
                                WINDOW_RES, WINDOW_RES,     // Resolution
                                0.1f, 2500.0f};				// Dist. to near and far clipping planes
    // Define transformation matrices
    const mat4 model_mat{1.0f};                             // Model to World space (Identity)
    const mat4 model_view{cam.computeModelView(model_mat)};	// Model to Camera space
    const mat4 MVP{cam.computeMVP(model_view)};			    // Model-view-projection matrix
    const mat3 norm_mat{cam.computeNormXForm(model_mat)};	// Matrix transforming normals
    // Load shader program
    GLSLProgram sp{};
    sp.loadShader("Source\\Shaders\\Render.vert");
    sp.loadShader("Source\\Shaders\\Render.frag");
    sp.link();
    sp.use();
    // Load scene
    scene = new Scene;
    scene->loadObjects("Assets\\cornell_box.obj", sp.id());
    // Set up fog
    scene->addFog("Assets\\df.3dt", "Assets\\pi_df.3dt", MAJ_EXT_K, ABS_K, SCA_K, cam);
    const BBox& fog_box{scene->getFogBounds()};
    const vec3 fog_pt_min{fog_box.minPt()};
    const vec3 fog_pt_max{fog_box.maxPt()};
    const vec3 box_top_mid{fog_pt_max - 0.5f * vec3{fog_pt_max.x - fog_pt_min.x, 0.0f,
                                                    fog_pt_max.z - fog_pt_min.z}};
    // Set up primary lights
    LightArray<PPL> ppls{1};
    PPL prim_pl{PRIM_PL_POS, PRIM_PL_INTENS};
    ppls.addLight(prim_pl);
    OmniShadowMap ppl_OSM{PRI_SM_RES, 1, MAX_DIST, TEX_U_PPL_SM};
    // Declare VPLs
    LightArray<VPL> vpls{MAX_N_VPLS};
    OmniShadowMap vpl_OSM{SEC_SM_RES, MAX_N_VPLS, MAX_DIST, TEX_U_VPL_SM};
    // Create a Halton sequence buffer object
    static const GLuint seq_sz{30 * 24};   // 30 frames with at most 24 samples per frame
    GLfloat hal_seq[seq_sz];
    HaltonSG::generate<2>(hal_seq);
    GLTextureBuffer hal_tbo{TEX_U_HALTON, gl::R32F, sizeof(hal_seq), hal_seq};
    // Set static uniforms
    sp.setUniformValue("cam_w_pos",       cam.worldPos());
    sp.setUniformValue("vol_dens",        TEX_U_DENS_V);
    sp.setUniformValue("pi_dens",         TEX_U_PI_DENS);
    sp.setUniformValue("ppl_shadow_cube", TEX_U_PPL_SM);
    sp.setUniformValue("vpl_shadow_cube", TEX_U_VPL_SM);
    sp.setUniformValue("accum_buffer",    TEX_U_ACCUM);
    sp.setUniformValue("halton_seq",      TEX_U_HALTON);
    sp.setUniformValue("inv_max_dist_sq", invSq(MAX_DIST));
    sp.setUniformValue("fog_bounds[0]",   fog_pt_min);
    sp.setUniformValue("fog_bounds[1]",   fog_pt_max);
    sp.setUniformValue("inv_fog_dims",    1.0f / (fog_pt_max - fog_pt_min));
    sp.setUniformValue("model_mat",       model_mat);
    sp.setUniformValue("MVP",             MVP);
    sp.setUniformValue("norm_mat",        norm_mat);
    ppls.bind(UB_PPL_ARR);
    vpls.bind(UB_VPL_ARR);
    // Init dynamic uniforms
    RenderParams params;
    InputHandler::init(&params);
    int tri_buf_idx = 0;
    GLUniformManager<9> uniform_mngr{sp, {"gi_enabled", "clamp_rsq", "exposure", "frame_id",
                                          "n_vpls", "sca_k", "ext_k", "sca_albedo",
                                          "tri_buf_idx"}};
    // Create ring-triple-buffer lock manager
    RTBLockMngr rtb_lock_mngr;
    /* Rendering loop */
    while (!window.shouldClose()) {
        // Start frame timing
        const uint t0{HighResTimer::time_ms()};
        // Process input
        InputHandler::updateParams(window.get());
        // Wait for buffer write access
        rtb_lock_mngr.waitForLockExpiration();
        // Update primary lights
        ppls.reset();
        prim_pl.setWPos(params.ppl_w_pos);
        ppls.addLight(prim_pl);
        // Update VPLs
        if (params.gi_enabled) {
            const vec3 shoot_dir{normalize(box_top_mid - prim_pl.wPos())};
            rt::PhotonTracer::trace(prim_pl, shoot_dir, params.max_num_vpls, vpls);
            // Disable GI on VPL tracing failure
            params.gi_enabled = params.gi_enabled && !vpls.isEmpty();
            if (vpls.size() < params.max_num_vpls) printError("VPL tracing problems.");
        }
        // Generate shadow maps
        gl::CullFace(gl::FRONT);
        const uint t1{HighResTimer::time_ms()};
        ppl_OSM.generate(*scene, ppls, model_mat);
        if (params.gi_enabled) vpl_OSM.generate(*scene, vpls, model_mat);
        // Set dynamic uniforms
        sp.use();
        uniform_mngr.setUniformValues(params.gi_enabled, params.clamp_r_sq,
                                      params.exposure, params.frame_num,
                                      min(vpls.size(), params.max_num_vpls),
                                      params.sca_k, params.abs_k + params.sca_k,
                                      params.sca_k / (params.abs_k + params.sca_k),
                                      tri_buf_idx);
        // RENDER
        gl::CullFace(gl::BACK);
        const uint t2{HighResTimer::time_ms()};
        window.clear();
        scene->render();
        // Switch to the next buffer
        rtb_lock_mngr.lockBuffer();
        ppls.switchToNextBuffer();
        vpls.switchToNextBuffer();
        tri_buf_idx = (tri_buf_idx + 1) % 3;
        // Prepare to draw the next frame
        window.refresh();
        params.frame_num += 1;
        // Display frame time
        const uint t3{HighResTimer::time_ms()};
        char title[TITLE_LEN];
        if (params.frame_num <= 30) {
            sprintf_s(title, TITLE_LEN, "GLGI (Render: %u ms | SM: %u ms | Misc: %u ms)",
                      t3 - t2, t2 - t1, t1 - t0);
        } else {
            sprintf_s(title, TITLE_LEN, "GLGI (done)");
        }
        window.setTitle(title);
    }
    return 0;
}
