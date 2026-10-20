# Terrain color, coastline and relief acceptance

Production terrain now has a direct normal/flattened comparison against original
native render buffers. Both complete 1024x511 scenes match every indexed pixel
and every RGB pixel: 1,046,528 pixels across the pair. The west-coast and east-ridge
regions independently match their original hashes, as does the complete 256-entry
Gamma100 software palette. This comparison required no saturation, contrast,
coast smoothing or arbitrary height adjustment in production drawing code.

The [generated startup acceptance](generated-terrain-appearance.md) extends this
to a natural coast and unknown-grid boundary, with both relief modes matching
every indexed/RGB pixel. The original-style generated map producer and ordinary
live generated-world snapshot connection remain implementation work.

The original control is retained wrapped 80x80 Planet at camera (22,38), raw zoom 0,
viewer 3 revealed, no Units/Bases, flags 0x10621, known grid off and variation
base-low16 0xbf60. It is a bounded renderer fixture, not ordinary startup or a
foreground original-game screenshot. Both original frames use identical source
banks, UVs, signed lights, palette and coast mappings; flattened terrain changes
geometry placement. The pair differs at 253,670 indexed pixel positions.

Original evidence comes from `terrain-appearance-evidence-v2-20261020`, manifest
SHA256 `d65a81759e49e6b1dd057c3aa49fda4d5aff19acfb3d5864cb41d8ab05f96d46`.
All 25 files were verified before import. Its standalone verifier independently
replays 2,469,355 actual original ordered stores and checks indexed/RGB buffers,
palette and PNG decoding. [Public acceptance metadata](behavior/terrain-appearance-acceptance-cases.json)
contains settings, source hashes and expected complete-frame/region hashes;
original assets and pixel/store buffers remain outside Git.

The production probe decodes owned PCX/map inputs using the existing preparation
tool, then invokes `Session`, `RenderPrepared`, the production software-palette
constructor and RGBA resolver. It compares indexed pixels and RGB separately.
It does not inject original draw commands, geometry returns or expected pixels,
and it does not exercise the application's complete SDL texture-loading path.

```sh
python3 tools/reference/verify_native_terrain_appearance.py \
  --asset-directory /path/to/purchased/game \
  --output-directory /tmp/glsmac-terrain-appearance --sanitize
```

This requires a C++17 compiler and SDL2 development files with `sdl2-config`.
Both cases pass normally and with ASan/UBSan across 13 standalone production/probe
units. The instrumented run disables leak detection; system SDL2 is not
instrumented. Generated native-size PNGs have no ICC/gamma metadata or resampling.

| Original native CPU buffer | GLSMAC production CPU buffer |
| --- | --- |
| ![Original normal relief](https://raw.githubusercontent.com/jhurliman/glsmac/c66dc4f8cee03f9d4bbf6a5ba637db9e9a46284a/terrain-appearance-media/original-normal_relief.png) | ![Matching GLSMAC normal relief](https://raw.githubusercontent.com/jhurliman/glsmac/c66dc4f8cee03f9d4bbf6a5ba637db9e9a46284a/terrain-appearance-media/glsmac-normal_relief.png) |

| Normal relief | Flattened terrain |
| --- | --- |
| ![GLSMAC normal relief](https://raw.githubusercontent.com/jhurliman/glsmac/c66dc4f8cee03f9d4bbf6a5ba637db9e9a46284a/terrain-appearance-media/glsmac-normal_relief.png) | ![GLSMAC flattened terrain](https://raw.githubusercontent.com/jhurliman/glsmac/c66dc4f8cee03f9d4bbf6a5ba637db9e9a46284a/terrain-appearance-media/glsmac-low_relief.png) |

## Live relief control

Ctrl+Shift+X now toggles the original "flattened terrain" option in native
gameplay views. It XORs More Preferences bit 0x10000 and selects base elevation
step 8 rather than 25 (native zoom scaling still applies). It preserves other
preference bits, raw zoom, camera, palette and submitted lighting. Repainting
uses the existing owned cache invalidation path and updates actor roots. The
game's underlying elevation data and console elevation measurements do not change.

The command is handled after UI input and retains its current view context
across world snapshots. It is not yet persisted by a profile/INI preference owner;
creating another view restores its current default. Original profile persistence
is still required. The installed original INI snapshot has flattening off, which
does not establish the settings of the user's Mac/VM reference session.

The cached native build and 56 affected native groups pass, including the actual
frontend key handler's round trip, preserved preference words/camera/zoom and
deferred committed-frame publication. GameNativeTerrain and its Game consumer
pass correctness analysis. A hidden ordinary Linux campaign run toggled normal
to flattened and back, then quit normally. No platform matrix was repeated.

## Live GPU color/readback check

A separate hidden Linux software-OpenGL run loaded Morgan 2105 (campaign SHA256
`aed85217e7684412e0de762d4ccf23df5892f20fc0355a8b2fa15ea55580ae92`). An external
test shim captured the actual 1024x511 RGBA texture upload and the RGB back buffer
after rendering. The previously observed actor rectangle [428,220,540,322) was
declared before comparison; it covers the Unit, Base, population and name drawn
over the terrain. It is not a mask derived from mismatching pixels.

Both modes have **zero different pixels outside that rectangle**. The ordinary
mode includes 38,818 colored terrain pixels there, flattened 37,654; both include
132 distinct RGB values. All uploaded alpha bytes are 255. The 3,688/3,689 differing
pixels fall wholly inside the actor rectangle. [Numeric readback receipts](behavior/native-terrain-gpu-readback-cases.json)
retain hashes, bounds and counts. These are one-off live integration observations,
not a committed general GPU regression harness or an original-game comparison.

![Actual flattened ordinary GPU readback](https://raw.githubusercontent.com/jhurliman/glsmac/c66dc4f8cee03f9d4bbf6a5ba637db9e9a46284a/terrain-appearance-media/live-flattened.png)

Both live runs exited 0 with no tracked OpenGL leaks. Audio and allocation-debug
tracking were disabled. This evidence finds no additional tint/resampling in the
tested Linux path. It does not prove physical monitor color, HDR/ICC behavior,
native Mac/Windows presentation, other gamma settings, every map/zoom or complete
actor/font fidelity. It also does not diagnose the older user screenshots: those
may use legacy terrain drawing, another scene or different relief settings.
