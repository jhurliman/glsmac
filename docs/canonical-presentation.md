# Canonical presentation

SMAC's measured 1024×768 screen has a 257-pixel lower console and a
511-pixel map region. The game now renders its complete world, UI, fonts,
movies and captures into one 1024×768 logical canvas. Changing the window
size does not change that layout or the camera's logical viewport.

Presentation uses the largest whole-number pixel scale that fits the actual
SDL OpenGL drawable, with centered black bars. A 1280×960 drawable contains
a centered 1× canvas; 2048×1536 contains a 2× canvas. Smaller drawables fit
4:3 with nearest downsampling; those cannot preserve every original pixel.
No interpolation or independent HUD/card resizing occurs in presentation.

SDL input uses window coordinates, which can differ from Retina drawable
pixels. The inverse map uses those actual dimensions and rejects the bars;
it does not clamp clicks onto edge controls. The source pixel under the
window pixel center is the hit-test authority. Normal resize leaves logical
geometry and camera framing unchanged.

Nested world-data and UI captures restore the canonical default target.
Effect backdrops read it as well. The debug resource tracker explicitly
models this default target independently from temporary framebuffer scopes;
ordinary textures and buffers are not owned by the presentation framebuffer.
The default target and render lock retire on an exceptional frame too.

This fixes presentation proportions, not all original game rendering. The
native indexed map's inspector/knowledge/zoom admission remains unchanged;
1024×768 logical sizing does not authorize revealing normal-game terrain.

Validation includes actual OpenGL nested capture/backdrop checks, all
4,854,528 presented pixels across 1280×960, 2048×1536 and 800×600 controls,
resize and failed-frame recovery, plus exhaustive 1024×768 Retina point
mapping and movie layout. Run the optional graphics control with
`GLSMAC_TEST_PRESENTATION_GPU=1` and native filter `^canonical OpenGL`.
Existing GPU controls that create another context run in separate processes,
as the debug tracker's existing context lifetime is process-scoped.
