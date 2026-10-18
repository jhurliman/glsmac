# GPU-resident UI cache

UI cache rebuilds previously rendered into the shared framebuffer, read the
cropped RGBA pixels into a CPU Texture, and uploaded that bitmap to another GPU
texture. Rebuilding large or nested UI widgets repeatedly crossed the CPU/GPU
boundary. Native Windows profiling identified UI-cache rebuilds as the location
of long frames; readback alone did not explain the whole stall.

Each graphics Cache now owns one cropped RGBA8 GPU texture, allocated only when
its dimensions change. The shared framebuffer renders children as before, then
`glCopyTexSubImage2D` copies the same rectangle directly into that texture.
Sampling, nearest filtering, repeat wrapping, coordinate orientation and blend
behavior are retained. This does not allocate a full-window framebuffer per
widget. Empty caches still traverse the rendering path but do not allocate
zero-sized storage or copy pixels. Destruction releases the owned GL texture
alongside the existing vertex/index buffers.

The existing CPU readback API remains for actual CPU consumers and captures.
GPU copy bounds are checked against the framebuffer before issuing the copy.
Window letterbox offsets apply only to the default framebuffer, preserving local
offscreen coordinates.

## Observed validation, 2026-10-18

A disclosed private installed-asset Linux SDL probe adds two overlapping
translucent child panels to a translucent parent and changes a text label every
250ms for 40 updates. It captures the final real backbuffer and repeats capture
after resizing from 1920x1080 to 1000x1000 and 1024x768. Original assets and
screenshots remain outside Git.

Before/after RGB images are byte-identical at all three sizes. Both application
processes exit0 with no GL errors, and the engine's OpenGL resource tracker
reports no leaked GL resources. This verifies this composition/resize workload,
not original-game visual parity or every UI/model capture path.

Before the probe's own readback, the old path issued181 `glReadPixels` calls
(total instrumented call time166.825ms); the GPU path issued0. The native cached
build and focused correctness analysis on OpenGL.cpp, FBO.cpp, Cache.cpp and
its Actor.cpp consumer pass. The changed path adds no CPU pixel allocation.

Measured Linux software-rendered loop intervals are diagnostic only: old
p50/p99/max2.214/3.649/7.376ms; new2.305/4.019/6.947ms. They do not demonstrate
an overall frame-rate improvement. The final repeat also ran alongside bounded
correctness analysis, so it is not a controlled speed comparison. The verified
result is elimination of UI CPU readbacks with identical pixels in this probe.

Windows Release foreground pacing, the measured native map hitch, GPU timing,
actual hardware audio underruns and representative late-game workloads remain
unverified. Do not claim that this change certifies VSync or fixes every native
stall. Reuse the native performance audit's workloads for that follow-up.
