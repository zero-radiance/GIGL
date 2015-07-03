#pragma once

#include "Definitions.h"

/* Mathematical constants */
#define TWO_PI			6.28318548f		// Single-precision value of 2 * PI
#define PI				3.14159274f		// Single-precision value of PI
#define HALF_PI			1.57079637f		// Single-precision value of PI / 2
#define INV_PI			0.318309873f	// Single-precision value of 1 / PI

/* Rendering settings */
#define WINDOW_RES      800
#define MAX_N_VBOS      8               // Maximal number of vertex buffers
#define MAX_DIST		1000.0f			// Camera distance to far plane
#define PRI_SM_RES      512             // Primary shadow map resolution in one dimension
#define SEC_SM_RES      64              // Secondary shadow map resolution in one dimension
#define N_GI_BOUNCES	3				// Number of light bounces for GI
#define MAX_N_VPLS      150				// Max. number of VPLs
#define MAX_N_FAILS		1000			// Max. number of failed attempts to trace a path
#define PACKET_SZ       8               // Ray packet size for packet tracing
#define RAY_OFFSET      1E-4f           // Offset in normal direction to avoid self-intersections
#define TRI_EPS         1E-4f           // Small epsilon value used by triangle intersector
#define SURVIVAL_P_RR	0.95f			// Survival probability for Russian Roulette
#define THRESHOLD_MS    500             // Used to ignore repeated key activations
#define TITLE_LEN		58				// Number of characters in window title
#define PRIM_PL_POS     {278.2f, 600.0f, 279.5f}     // Primary point light position
#define PRIM_PL_INTENS  {85000.f, 75000.f, 75000.f}  // Primary point light intensity

/* Fog properties */
#define ABS_K           1E-5f           // Absorption coefficient per unit density
#define SCA_K           1E-2f           // Scattering coefficient per unit density
#define MAJ_EXT_K       0.010001f       // Majorant extinction coefficient
#define MAX_FOG_HEIGHT	548.8f			// Height limit of fog (for VPLs)
#define HG_G            0.25f	        // Henyey-Greenstein func. scattering asymmetry parameter
#define N_OCTAVES		6				// Number of octaves for simplex noise

/* Texture unit allocation */
#define TEX_U_DENS_V	0               // DensityField (for fog)
#define TEX_U_PPL_SM	1               // Primary point light (PPL) shadow map
#define TEX_U_VPL_SM	2               // Virtual Point Light (VPL) shadow map
#define TEX_U_ACCUM		3               // Accumulation buffer texture for progressive rendering
#define TEX_U_PI_DENS   4               // Preintegrated fog density values

/* Uniform locations */
#define UL_SM_MODELMAT	0               // Model matrix
#define UL_SM_LIGHTMVP	1               // 6 of them: 1..6
#define UL_SM_LAYER_ID	7               // layer_id
#define UL_SM_WPOS_VPL	8               // VPL position in world coordinates
#define UL_SM_INVMAXD2	9               // Inverse max. distance squared

/* Uniform binding indices */
#define UB_MAT_INFO		0               // Material description
#define UB_PPL_ARR		1               // Primary point light (PPL) array
#define UB_VPL_ARR		2               // Virtual Point Light (VPL) array

/* Misc. OpenGL definitions */
#define GL_FALSE		0				// gl::FALSE_
#define GL_TRUE			1				// gl::TRUE_
#define DEFAULT_FBO		0				// Rendering framebuffer handle
