# Generated startup coast, relief and unknown-boundary acceptance

Production terrain now matches an independently generated original startup scene
in both normal and flattened relief: **zero differing indexed or RGB pixels**
across the two complete 1024x511 terrain frames (1,046,528 pixels). This extends
the [retained Planet comparison](native-terrain-appearance.md) to a naturally
generated coast and explored/unknown boundary. No production tint, coast smoothing
or height adjustment was required.

These are terrain-pass crops of the original 1024x768 gameplay layout. Objects,
names and the lower console are outside the captured pass. They are not complete
application screenshots or a claim that generated GLSMAC games already use this
authoritative state.

| Original normal-relief CPU buffer | GLSMAC production CPU buffer |
| --- | --- |
| ![Original generated startup](https://raw.githubusercontent.com/jhurliman/glsmac/cc6f1475d6575c3bbc2789a535fa42640d111d55/generated-terrain-acceptance/original-normal.png) | ![Matching generated startup](https://raw.githubusercontent.com/jhurliman/glsmac/cc6f1475d6575c3bbc2789a535fa42640d111d55/generated-terrain-acceptance/glsmac-normal.png) |

| Original flattened CPU buffer | GLSMAC production CPU buffer |
| --- | --- |
| ![Original flattened startup](https://raw.githubusercontent.com/jhurliman/glsmac/cc6f1475d6575c3bbc2789a535fa42640d111d55/generated-terrain-acceptance/original-flattened.png) | ![Matching flattened startup](https://raw.githubusercontent.com/jhurliman/glsmac/cc6f1475d6575c3bbc2789a535fa42640d111d55/generated-terrain-acceptance/glsmac-flattened.png) |

## Original producer and paint boundary

Frozen bundle `generated-world-terrain-evidence-20261020` has manifest SHA256
`45099a19fa607e492dbfba02f810cbadfc0b8dc3086418b2af2bb7a4e821aeaa`;
all 34 files were verified before import. The pinned executable hash and compact
[numeric input/acceptance fixture](behavior/generated-world-terrain-acceptance-cases.json)
retain the provenance. Purchased art and complete original buffers stay outside
Git.

Original setup at 0x5a0b80 runs complete generator 0x5e0100 and seven-faction startup
0x5cadb0 through 0x5a1f2d. The supplied 80x80 Standard Rules world uses seed 1,
size category 2, generation categories 1, difficulty 3 and human faction 1. Terrain, start sites and
exploration are original outputs. Setup includes actual layout/search scratch
initialization; allocating tile records alone is insufficient. The completed
turn 0 state has seven Bases, eleven Units and active mask 255, including native
life bit 0. Human Base/Scout start is (25,35). Each faction naturally knows 21 tiles;
the human knows three sea tiles. There is no supplied reveal radius.

A separate original renderer execution receives those complete 44-byte tile
records, faction/global state and prepared landmark profiles. It paints through
0x4766db, immediately before the object loop, at camera (25,35), zoom 0, viewer 1,
flags 0x60010ee3 and missing-INI difficulty 3 preferences 0xBBE1DD96/0x003A7168. Both
frames start with the same x87 control word. Flattening changes only the option
bit and relief step 25 to 8. Render-machine allocation low16 0xbf60 is an explicit
material-variation input; producer allocation low16 0x1290 is distinct.

Each original frame contains nine coast raster calls and 1,068 unknown-grid calls.
The independent original checker verifies 1,228,185 ordered stores across both
frames, caches, input hashes and indexed/RGB buffers. The port check compares
final frames and the natural coast/boundary region; it does not compare the port's
ordered-store trace or cache bytes.

## Reusable port check

```sh
python3 tools/reference/verify_generated_terrain_appearance.py \
  --asset-directory /path/to/purchased/game \
  --output-directory /tmp/glsmac-generated-terrain --sanitize
```

The command validates owned asset hashes and reuses the production appearance
probe. It first checks both earlier retained-Planet relief controls, then draws
both generated startup controls through `Session`, observer knowledge,
`RenderPrepared`, the production software palette and RGB resolver. It requires
a C++17 compiler and SDL2 development files. Output PNGs preserve native size
with no resampling or ICC/gamma metadata.

By default, the fixture reconstructs exactly the 6,390-byte conservative observed
field/tile envelope from the original pass, with other fields zero **only in
this bounded diagnostic**. The full original records were independently tested
and produce exactly the same accepted frames. This is not a permission to discard
unobserved geography in gameplay or a universal minimal-input proof. The optional
`--generated-records /path/to/seed1.tiles.bin` validates the full 140,800-byte
producer output hash and tests it directly instead.

Both generated frames pass using the complete original records. Both also pass
using the public bounded envelope under a focused 13-TU ASan/UBSan standalone
build; SDL is uninstrumented and leak detection is disabled. The retained pair
passes in both runs, covering the probe's existing mode. No engine/platform
matrix was repeated for this diagnostic addition.

## Remaining gameplay work

The [native pretemperature pipeline](native-pretemperature-generation.md) now
reproduces actual setup through its first temperature entry, including retries.
Later generation stages and startup remain to be connected.

The port's legacy procedural generator does not reproduce the original complete
44-byte generated records and stage ordering. Ordinary native map authority is
still connected to retained Planet. Generated-world preparation and its live
snapshot adapter must be implemented before these images can be claimed for
normal generated gameplay. Flat/arbitrary map profiles, actor/font/UI fidelity,
native Mac/Windows GPU output and physical display color remain separate gates.
The current result rules out a terrain renderer mismatch in these controlled
scenes; it does not diagnose every older Mac screenshot.
