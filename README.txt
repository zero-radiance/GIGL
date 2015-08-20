GIGL - Tiny Global Illumination OpenGL Renderer

DISCLAIMER: this is a work in progress! :-)

Loosely based on the following papers:
1. Keller, "Instant radiosity";
2. Engelhardt et al., "Instant multiple scattering for interactive rendering of heterogeneous participating media";
3. Engelhardt et al., "Approximate bias compensation for rendering scenes with heterogeneous participating media".

Requirements:
1. MSVC2013 redistributables (https://www.microsoft.com/en-us/download/details.aspx?id=40784);
2. graphics drivers with OpenGL 4.4 support.

Notice:
During the first start up, the renderer will procedurally generate volume density textures (64^3 in the world space, and 64x1024^2 precomputed in the camera space). This may take a few seconds.
The subsequent program launches will be considerably faster, as the textures will be read from the disk.

Controls:
* Arrow keys    move the primary light source;
* Numpad + -    control the volume density;
* Numpad * /    control the exposure time;
* F             toggle the fog;
* G             toggle global illumination;
* SPACE         reset all settings;
* R             reset the accumulation buffer (hold to temporarily disable it);
* C             toggle clamping and the primary light source;
* 1             set the VPL count to 10;
* 2             set the VPL count to 50;
* 3             set the VPL count to 150;
* ESC           exit the application.
