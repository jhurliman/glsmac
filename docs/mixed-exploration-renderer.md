# Mixed explored and unknown terrain rendering

The production native terrain renderer now reproduces the complete original
partial21 view: first paint, warm repeat, east pan and return pan. All four
1024x511 indexed frames match (2,093,056 pixels). This is an original supplied
Planet fixture, not a natural startup or an ordinary-game presentation claim.

## Correct material authority

The center tile's selected/remembered improvement word controls admission of
Farm, Forest and Fungus. Connectivity for neighboring Forest and Fungus instead
reads live neighbor items, regardless of their exploration/memory. The renderer
previously applied the center memory policy to every neighbor. That changed
1,559 pixels in the first mixed frame. Keeping center selection separate from
live neighbors removes every discrepancy in all four frames.

Both land material and shelf Fungus APIs now accept an explicit optional center
word, while their lookup supplies live neighbors. An explicit zero suppresses a
live center feature. The console material producer uses the same distinction;
it no longer requires neighbor improvement memory to paint a known Fungus tile.
Its center inspection gate and separate Road memory policy remain in force.

The original [material/survey receipt](behavior/partial21-material-survey.md)
pins center tile+24 versus neighbor tile+8, the exact variation base0xbf60,
resource pattern27067 and authored memory words. The180-file bundle was verified
against manifest SHA256
`bf96823aa7d5ea199e7dc9bce0b1641e8fb3ee8650802d65a9664a236e003aef`.
The receipt also documents unfinished survey semantics; importing it does not
implement or close Unity Survey.

## Reproducible production comparison

```sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/reference/capture_native_viewport.py \
  --asset-directory '/path/to/Alpha Centauri' \
  --output-directory /tmp/glsmac-partial21 \
  --variation-low16 48992 --initial-index 65 --viewer 3 \
  --exploration-profile partial21 --sanitize
```

The tool verifies original assets, clears exploration/occupancy bits, sets the
supplied21 footprint, preserves authored faction3 memory and checks the complete
record transform hash. It feeds purchased raw map/art to actual production
material/geometry/raster code. Captured commands and pixels are comparison
targets, not inputs. Generated art-derived images remain outside source Git.
The revealed profile remains the default; mismatched viewer/resource/backdrop or
receipt profiles are rejected.

The four previous revealed-frame hashes and paint-store counts still match
under ASan/UBSan. Eleven affected native groups pass, including original texture
calls, coast commands, full unknown buffers, preview ownership and the new
remembered-center/live-neighbor regressions for land Forest/Fungus and shelf
Fungus. Five production translation units (changed units and the geometry caller)
pass correctness analysis. A focused seven-TU ASan/UBSan native run also passes;
other objects are uninstrumented and leak detection is disabled.

The image below is actual production indexed output resolved through the verified
original active palette. It is not an ordinary-game framebuffer screenshot.

![Original-matched production mixed exploration view](https://raw.githubusercontent.com/jhurliman/glsmac/18c4324de86f42546388854f77e9fb2660b1ce3b/mixed-exploration-view/first.png)

Ordinary main-map renderer integration, survey, foreign entities, other map sizes
and cameras remain open in [the integration plan](main-map-exploration-integration.md).
No local platform matrix was repeated for the material-input correction.
