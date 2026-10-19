# Conventional Scout playback development checkpoint

The actual admitted conventional attack prepares an owned presentation request
from its resolved round history before committing the result. Gameplay commits
casualties, surviving state, hostility, palette and RNG in one transaction.
Playback never advances combat or consumes RNG; save/Start retains the committed
result without replaying presentation.

The frontend queues subsequent requests behind the battle, keeping both Unit
display identities until death frames and tail finish. Health snapshots precede
grouped hit effects; all 16 frames render in order. Late presentation ticks do
not skip frames or catch up. Audio actors finish independently of frame tails.
World cleanup retires queued payloads and stops battle audio. Input and the
ordinary Unit animation/work pump are inhibited during battle. Disabled audio
skips waveform loading and actor creation.

Original XI/XF crops, hit/death sound basenames, grouped damage and normal waits
are connected. The pure clock supports measured Fast Battle waits; the frontend
currently uses normal mode, pending preference wiring. Installed game assets
remain local and are not redistributed.

Effects now use destination-dependent CLUTs generated from SYSTEM.TXT banks 3
(hit) and 0 (death). Indexed source pixels, CLUT and common palette are packed
into a retained texture. The GPU copies the current framebuffer, reconstructs
an ordinary palette index and applies the source-selected lookup row. Source
253/255 is transparent; source 0 is a real effect row. This removes the earlier
incorrect dependence on the embedded XI/XF RGB palettes.

Two fidelity limits are explicit. RGB-to-index reconstruction is verified for
ordinary palette-backed backgrounds, not the entire lit world renderer or
reserved background indices. XF contains 210 source-index-252 pixels that the
original reads outside its allocated bank. Their stock initialized colors are
unresolved; this port temporarily renders those pixels transparent. Neither
limit is a claim of original pixel fidelity.

Validation:

- Original 19-round seed-3 history, six grouped hits, death and 112 frames match
  the numeric receipt. Normal/fast and late-clock controls and payload deep-copy
  ownership pass.
- Actual hostile Scout and defended-Base commands emit timelines while existing
  results, RNG, casualty wrappers and save/Start checks pass.
- CPU controls cover truncation, nearest-color ties, protected/reserved indices,
  transparency, invalid rows and the documented index-252 rejection.
- An opt-in hidden SDL/OpenGL test uses the production shader and framebuffer
  copy and compares every ordinary destination index against all 16 source rows,
  plus transparency. The test passes with debug GL lifetime tracking enabled.
- Fifteen affected production translation units passed correctness analysis;
  final MemoryWatcher/OpenGL changes passed a further two-unit analysis.
- The five focused groups pass ASan/UBSan with 15 production/test translation
  units instrumented and remaining current normal objects reused. Leak detection
  is disabled. The GPU control uses software GL and proves neither hardware
  frame rate nor audible playback.

Remaining before review: actual graphical battle capture and cancellation,
projection/zoom alignment, audible sound/channel behavior, preference/skip
policy, and resolving or explicitly accepting the documented compositor limits.
Separate CVR body/facing clips and broader combat domains remain open.

[Original audiovisual contract](scout-combat-audiovisual.md).
[Original indexed compositing contract](scout-effect-indexed-compositing.md).
