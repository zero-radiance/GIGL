GIGL - Global Illumination OpenGL Renderer

Loosely based on the following papers:
1. Instant multiple scattering for interactive rendering of heterogeneous participating media
2. Approximate bias compensation for rendering scenes with heterogeneous participating media

Requirements:
1. MSVC2013 redistributables (https://www.microsoft.com/en-us/download/details.aspx?id=40784);
2. graphics drivers with OpenGL 4.4 support.

Controls:
* SPACE      reset all settings;
* G          toggle global illumination;
* C          toggle clamping and primary light source;
* F          toggle fog;
* R          reset accumulation buffer;
* 1          set VPL count to 10;
* 2          set VPL count to 50;
* 3          set VPL count to 150;
* Arrow keys move primary light source;
* Numpad +,- control volume density;
* Numpad *,/ control exposure;
* ESC        quit application.
