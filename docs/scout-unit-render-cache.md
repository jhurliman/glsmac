# Owned Scout unit sprites and previews

This connects the [owned Scout renderer](scout-idle-renderer.md) to the existing
static UnitDef, frontend Unit/GetSprite, and registered unit-preview paths. It
adds an explicit `cvr_scout` render choice; no default unit definition, movement,
combat, or Mind Worm identity changes.

## Descriptor and placement

The render descriptor names one logical composition: the reactor-1 flat-idle
Scout. It contains no paths. The self-contained typed record has marker
`0x43565253`, version 1, composition 0, four signed32 view fields, and four
binary32 placement fields. Unknown profiles, incorrect types, truncation,
trailing bytes, and records larger than `MAX_WIRE_BYTES` (256) reject. The Render
dispatcher applies that same limit to the enclosing string frame before payload
allocation or cursor advancement, using the bounded Buffer reader prerequisite.

Every field is required by the native `define_unit` API. Facing is -1 through 7,
zoom is -8 through 8, and the scale denominator is positive. The signed32
numerator retains the renderer's nonpositive-ratio zoom fallback. Positive
ratios must also satisfy `floor(50 * numerator / denominator) <= INT32_MAX`,
computed with a widened product and checked after division. This is the derived
anchor added to the texture path's fixed zero anchor; constructor, script
definition and snapshot admission all reject overflow before frontend rendering.
A nonzero division remainder may still produce an admitted INT32_MAX quotient.
World width and height must be finite, positive, no greater than 16, and have a
nonzero half-size. Image centers must be finite and within 0 through 256.
Derived offsets and rectangle extents must remain finite. These are explicit
port admission limits, not original game world-unit measurements.

The complete 256-by-256 image uses UV [0,1] on both axes. Sprite geometry is
centered, and the actual frontend Sprite adds destination offsets when placing
an instance. A requested source center `(cx,cy)` therefore produces offsets
`((0.5-cx/256)*width, (0.5-cy/256)*height)`. This does not reuse the PCX cell
offset convention or crop transparent borders. Preview normalization continues
to use the existing frontend mesh and widget geometry path.

## Asset and texture ownership

The scene's sprite manager resolves fixed names `vi.cvr` and `viptr00.cvr`
through ResourceManager. Each file read is bounded to 32 MiB; both owned inputs
and all renderer validation must succeed before the immutable asset set is
published. Assets are loaded once per scene cache. There is no in-place reload;
a replacement scene/cache resolves fresh assets. Failed loading or generation
does not replace a previously usable texture or asset set.

The cache keys all four pixel-affecting view inputs and admits at most 64
successful views per scene, without eviction. Placement and definition identity
separately key instanced actors: different definitions retain independent
instance counters while sharing identical pixels. Float placement keys preserve
their exact stored bit patterns. Anchor metadata is fixed at zero for this
texture path and does not select different pixels.

Generated textures carry their tracked deleter through shared ownership from
the cache to InstancedSprite, Unit render data, and each preview widget. The
widget retains the source before creating actors that borrow it. Scout previews
also own independent copies of the badge and healthbar textures, whose original
owners are different managers. Widget clear/destruction destroys its borrowing
actors before releasing these texture owners. Ordinary PCX previews retain
their existing behavior.

A registered preview can outlive its presenter. Its update/remove closures use
a weak presenter-lifetime token checked before any presenter or Game access.
The token expires before the base presenter unregisters. Reattachment replaces
the old update closure and retains the existing immediate-refresh behavior.
This covers sequential UI teardown/rebind; it is not synchronization for
concurrent presenter deletion.

## Validation boundary

Synthetic CVR inputs exercise the complete installed-name resolver, asset
readers, renderer and texture producer without proprietary assets. Native tests
cover descriptor rejection/placement, every pixel-key field, the cache limit,
failed replacement, actual read-only/invalid/valid `define_unit`, UnitDef wire
roundtrip, independent UnitDef actors, and actual Unit placement. The preview
fixture uses the real DOM factory and registration, game widget unregistration,
retained updates, reattachment, manager teardown, and parent-driven destruction.
Only graphics and badge assets are doubled; this is not a rendered OpenGL or
playable-match test.

An opt-in installed-asset smoke runs the same Unit/preview test by setting
`GLSMAC_SCOUT_ASSETS` to the installed game directory. It reads the two assets
without changing the installation. Pixel equivalence to original execution is
the separately documented renderer receipt; this adapter does not claim a new
original-game comparison.

On source parent `a4dfd992` plus the standalone bounded-reader prerequisite
`f65700c5` (cherry-pick of `b493406c`), measured Linux validation is:

- Full normal CTest: 8/8, 75.51 seconds.
- Full ASan + UBSan + LSan CTest: 8/8, 142.03 seconds, with leak detection and
  halt-on-error enabled and the existing local non-PIE workaround.
- All three new native groups pass with allocation tracking in both builds.
  After strengthening the unload observer, the normal fixture was rebuilt and
  rerun; the sanitizer full run already includes that observer. The expanded
  12-group tracked ownership CTest also passes in both builds (1.04 and 4.17
  seconds). This targeted rerun includes the final CMake selector change.
- The opt-in actual Unit/registered-preview path passes against installed
  `VI.cvr` and `Viptr00.cvr` in both normal and sanitizer builds. The synthetic
  badge loader and Null graphics remain in place.
- Five independently compiled copied-production controls exit 1: remove the
  outer string cap, remove the retained remove-callback token check (UBSan
  detects access through a destroyed presenter), omit preview texture retention,
  keep the old update closure on rebind, or omit facing from the pixel cache key.
  Intended source and normal object files were never replaced by these controls.

Full production correctness analysis (LLVM 15, shared-header/CMake scope)
passes all 322 translation units with zero failures in 237.94 seconds.

Retained local evidence is under
`/home/jhurliman/src/glsmac-resource-callback-audit/`: the
`scout-cache-bounded-{final-normal,asan}-ctest.log` receipts, final observer and
ownership-CTest logs, installed smoke logs, and
`scout-cache-negative-controls.json` with its per-control logs/copied TUs.
Earlier fixture setup failures (missing factory initialization and duplicate
parent/child destruction) remain separate from the corrected receipts; neither
is counted as a production negative control. The earlier unbounded-dispatch
checkpoint also passed both suites and is retained separately.

No native Windows/macOS run, rendered preview, hot-reload transaction, or live
gameplay Scout replacement is claimed by this adapter receipt.

The follow-up anchor admission repair on source parent `cdfc2a51` passes normal
CTest 8/8 in 72.48 seconds, all three affected tracked groups in normal and
ASan/UBSan/LSan, and the installed-asset Unit/registered-preview case with
sanitizers and tracking. Changed-production-TU correctness analysis has zero
failures in 1.85 seconds. A separately compiled original `CvrScoutRender.cpp`
with the new tests gives one pass/two failures at the constructor and actual
script/UnitDef snapshot admission assertions. The exact quotient-with-remainder
boundary remains accepted through MakeView and both codecs; this is descriptor
admission coverage, not an assertion that arbitrary extreme scales rasterize
within the bounded surface. Logs are retained under
`/home/jhurliman/src/glsmac-scout-anchor/`. This follow-up does not claim another
full sanitizer-suite or native platform run.
