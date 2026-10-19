# Populated original Planet viewport

Eight complete indexed controls extend the valid revealed-faction-3 Planet scene with three University bases, one garrison Scout, and one visible held Scout. The original viewport executes terrain, object iteration, base art, model composition/raster, final model-to-viewport copy, flags, health bar, population boxes, and glyph requests. The exact reduced inputs and results are in `viewport-populated-cases.json`; owned pixels and streams stay private.

Use **flags `0x106e1`**, zoom 0, viewer/display faction 3, faction all-map revelation bit `0x200`, all tile exploration masks 8, camera `(22,38)`. The previous empty-roster `0x10621` controls exclude native object drawing; `0xc0` enables the ordinary unit/base paths. Do not keep those old flags and inject object draws outside the dispatcher. Names are not enabled in this fixture.

| Record | Tile | Owner / former | Population / damage | Other state |
| --- | --- | --- | --- | --- |
| Base 0 | 20,36 | 3 / 3 | Population 8 | No facilities; garrison Scout 0 |
| Base 1 | 24,38 | 3 / 3 | Population 9 | HQ facility 1 |
| Base 2 | 28,40 | 3 / 3 | Population 10 | No facilities |
| Scout 0 | 20,36 | Owner 3 | Damage 3 | Hold order 2, facing 0, home 0 |
| Scout 1 | 24,40 | Owner 3 | Damage 6 | Hold order 2, facing 2, unhomed |

Both Scouts use installed predefined design 2, morale byte 1, visibility mask 8, no movement spent, no stack links. The receipt pins all 304-byte base records, both 52-byte unit records, design 2, facility bits, remembered populations, and the exact changed tile records. Apply the listed tile changes to the imported raw Planet fixture in order; preserve its terrain, landmarks, live improvements, and other retained bytes. This is an explicitly declared roster, not a naturally founded three-city save or faction-start claim.

The original object anchors are `(432,191)` for Base 0, `(624,239)` for Base 1, `(816,275)` for Base 2, and `(624,288)` for visible Scout 1. They come from the actual current-camera projector/object pass. No extra screen projection or fixed flat-land anchor is supplied. The garrison Scout is not separately painted over Base 0; its unit marker makes the population box filled. Bases 1 and 2 have unfilled boxes. HQ changes Base 1's art size without adding walls.

The first/repeat native requests are population `8` at `(435,205)`, `9` at `(627,253)`, Hold `H` at `(635,304)`, and `10` at `(819,289)`. Population font is Arial bold logical height -16; Hold font is Arial bold height -12. The receipt includes exact native LOGFONT requests, host metrics, string widths, baseline/alignment, RGB and raw COLORREF, clip, boxes, sprite trim/source hashes, and native draw order. Health has black backing `[630,294,632,315)` and red index 249 fill `[630,294,632,302)` for damage 6 of reactor-1 health 10.

The terrain-complete first-frame boundary is exactly the prior valid-faction empty-roster hash `6f0767ae1eb7cc6c3d4658e9bcfd6ab0166295b32f3a31973f219859d4b65cd0` with 1,236,125 stores including clear. Objects then add 12,822 native target stores. The visible Scout executes 59 model frame records and 65 native raster kernels through the real final target copy. The initial software-3D color is key index 9, depth 65535; both native color/shadow bitmaps are 256 by 256. The native context initializer fragment and cleanup execute. No cached model image or supplied pixel result is injected.

Two separate first/repeat/east/back sequences are captured. The indexed sequence suppresses final TextOutA painting while retaining actual host font measurements and every native box/model/terrain store. The presented sequence performs the actual glyph requests on a current Windows 8-bit GDI DIB with the same palette, interleaved with native painting. It is a host-font result, not historical Windows font parity. Font deltas are stored separately from native CPU stores. Neither sequence is formed by layering a final image onto an unrelated screenshot.

| Sequence | First, repeat, and back SHA256 | East SHA256 |
| --- | --- | --- |
| Indexed without font ink | `5eaf4b0306d1d3e9af7a177c70bc7760ed479c43c971bef9c00c44f7e8f96347` | `89267ccff3c6b73980768e309263584ee90c55546033fe5b5942df85ee1ca4f5` |
| Presented host fonts | `d899b01f70605a92432f3f04b5ac92aeaaaac0996a4dfe22e8174fd55cc58398` | `cad307c681b17dba7f55cdc6464c9d940ae56eabd2189d7536cc1cc48ec93ce3` |

Native store counts per sequence are 1,248,947; 1,248,947; 544,622; 545,466. Pan uses the original wrapper and self-scroll. East admits Base 2 in the expanded object dirty region; its population text request is clipped to `[928,0,1024,511)` and paints no glyph pixels. Back admits no object redraw, yet restores the complete first image. Keep those real clip/admission effects; redrawing every object after pan would not reproduce the ordered store receipt.

The checker exactly replays **7,175,964 native target byte stores** plus **478 host glyph byte changes**, verifies every frame, per-root stream hash, cache fingerprint, visible dispatch arguments and model record/kernel counts. A fresh process with clean native initialization independently reproduces the first indexed frame and its complete ordered-store hash. This verifies native receipt integrity and repeatability; it does not independently reimplement the complete city/model renderer or establish GLSMAC parity.

Pixel size is 1024 by 511, pitch 1024, initial backing 73, actual native clear 65, explicit unparented window profile. Map allocation low 16 bits are `0xbf60`. Model facing and animation counter are pinned; no clock API calls occur during the eight paints. Installed asset hashes, active palette hash and host Arial Bold file hash are pinned. Memory/file services, rectangle APIs and current Windows font APIs are explicit boundaries. Actor-cache UI notification remains excluded. Nested legacy query diagnostics are disabled; original native geometry and rule routines execute. Broader unit stacks, selected-unit blinking, hostile ownership/intelligence, base names, other models and complete original GUI presentation remain outside this finite fixture.
