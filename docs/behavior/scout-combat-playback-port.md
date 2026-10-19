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
reserved background indices. XF source252 uses an explicit measured initialized
SMAC software compatibility row, with the original reserved output RGB values
and opaque alpha. This safely reproduces that profile without reading unrelated
memory; SMAX and other bitmap/display lifetimes are not claimed.

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

Installed-asset graphical validation now also covers a genuine two-participant
battle in an external capture build of checkpoint85b102af. The only application
source difference is AddSinglePlayerSlot additionally calling CreatePlayerSlot
for a University peer with Role::Player, unique GSID/CID and the current
Librarian difficulty before normal startup. All remaining objects are the cached
normal build. The external worldscript uses real starting Base/Scout commands,
finds adjacent is_clear_land sites, accepts pending research through the real
choose_research command, selects the local Scout and submits move_unit against
the peer. This is a capture fixture, not a shipped opponent/AI setup feature.
The ordinary executable still creates only the local faction.

The first completed battle capture renders hit effects, health changes and an
XF death explosion using the user's installed XI/XF and WAV assets. A further
seeded1:2:3:4, size1, erosive100/lifeforms0 run waits until effect drawing has
stopped for three seconds, and shows the local casualty removed, the foreign
survivor retained, selection repaired and the deferred queue drained. Its
process exits0 with no tracked OpenGL leaks. A separate actual window-close
control injects SDL_QUIT after the fourth indexed-effect framebuffer copy on
texture unit2; it also exits0 with no tracked OpenGL leaks. These runs do not
enable heap tracking or establish audible output, hardware performance, exact
original projection or a full original-executable screenshot diff. Software GL
and periodic readback are capture machinery, not a frame-rate benchmark.

The index252 follow-up now establishes a measured initialized SMAC software
compatibility mapping and reserved RGB colors, including opaque cyan for the
background44→index6 case. The renderer now uses that explicit profile instead of the transparent
fallback. All256 outputs match the original-execution fixture. The GPU control
also renders the extra row, checking reserved RGB and alpha255; the five focused
groups pass again under ASan/UBSan, and the three changed production TUs pass
correctness analysis. SMAX initialization,
physical display behavior and later bitmap lifetime remain separate open limits.
See [index252](scout-effect-index252.md) and
[reserved software RGB](scout-effect-reserved-rgb.md).

Remaining before review: final public integration checks,
projection/zoom alignment, audible sound/channel behavior, preference/skip
policy, and resolving or explicitly accepting the documented compositor limits.
Separate CVR body/facing clips and broader combat domains remain open.

[Original audiovisual contract](scout-combat-audiovisual.md).
[Original indexed compositing contract](scout-effect-indexed-compositing.md).
