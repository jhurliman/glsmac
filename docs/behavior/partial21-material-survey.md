# Mixed partial21 material inputs and Unity Survey

The four full native mixed-known viewport frames are now reproduced by an independent primitive raster checker using the user's installed Planet/art. Eleven additional single-tile controls pin a minimal Unity Survey profile. The checker regenerates spans and pixels from captured primitive inputs; it does **not** independently produce material commands. The port should run its own material producer from the supplied raw records and context, then compare commands/pixels as expected outputs.

[Inputs and pixel controls](partial21-material-survey-cases.json) include each of21 raw44-byte center records, eight raw neighbors per center, variation seeds/results, selected words, actual helper arguments/results, ordered material primitives and complete frame hashes. [Pinned instructions](partial21-material-survey-source.json) cover center selection, live neighbor accesses and survey/shader branches. Native full frame/store bytes and receipts are in the separate frozen bundle `tmp/reference/partial21-material-survey-evidence-20261020`. Existing grid/console evidence remains unchanged.

## Exact mixed fixture

Pinned original `terran.exe` SHA256: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Original code executes under bounded CPU emulation with generic memory/window/file/GDI services and empty object rosters. No foreground game is touched. This is a supplied viewport fixture, not a complete natural launch/save-load receipt.

Import80×80 Planet.MP: records start at file offset2739,3200 records of44 bytes. For each record, clear exploration byte+4 and clear the two live occupancy bits (`byte+8 &= 0xfc`). Set only viewer3's exploration bit8 on the21 offsets around `(22,38)` from `first-worker-map-producers-cases.json`. Preserve all authored memory words. Resulting map SHA256 is `cc730e4eb644a6656123da09f0de9ad3b7aca0acec9068940c025080a3acc493`.

The authored remembered3 word is nonzero at just three cells:

| Cell | tile+24 remembered3 |
|---|---:|
| `(21,35)` | `0x000000a0` |
| `(19,37)` | `0x00004020` |
| `(24,38)` | `0x40100000` |

The other18 remembered words are zero. Their live improvements are often nonzero. Do not substitute all-zero memory or snapshot current improvements for this fixture. Natural startup's radius producer separately snapshots live improvements while granting exploration; that existing behavior is documented in [first-worker-map-producers.md](first-worker-map-producers.md).

Viewer/display faction3; status0; rules `0x1a0e`; resource pattern27067; viewflags `0x60010ee3`; zoom0; camera `(22,38)`; preferences `959f9c=0`; grid preference `959f98=0`; editor0; inactive scale override `[0,-1]`; site coefficients `[11,61]`; initial backing73;1024×511 indexed canvas. No clock deliveries occur during the viewport controls. The map uses a declared bump allocation at `0x4085bf60`, so **variation base low16 is `0xbf60`**. This is an exact harness input, not a portable claim about the natural Windows heap.

For a cell `(x,y)`, record index is `40*y+x//2`; variation seed is `(0xbf60+44*index)&65535`; variation is `((((seed*1664525+1013904223)&65535)*2)>>16)`. It consumes no gameplay RNG draw. Resource/site helpers use the imported pattern27067 and raw game inputs; their actual outputs are included, rather than guessed from remembered Supply Pod/resource bits.

## Center items and neighbor items have different authority

`46ff66` reads selected center word at tile+`12+4*viewer`, hence+24 for viewer3. Negative viewer or faction status `0x200` instead selects live tile+8 (`46ff6c`). Center Forest/Fungus admission subsequently tests local selected word (`47183e`, `471943`).

Neighbor connectivity reads **live**, even when the neighbor is unexplored or has different remembered features. Fungus's8-neighbor loop tests byte tile+8 bit`0x20` (`47067b`) and requires band>=2 (`470688`). Forest's4-neighbor loop tests byte tile+10 bit`0x20` (`4718cf`), equivalent to live DWORD tile+8 bit`0x200000`. These neighbor loops do not index faction memory or apply an exploration gate. Retain normal wrap/pole/flat-map bounds from the existing selector contract. Neighbor geography, moisture, contour and river connections likewise retain their established live-record inputs.

Expose selected center items and live neighbor items as separate producer inputs. Passing a single remembered-world lookup to the material selector cannot reproduce this scene.

## Full mixed frame controls

| Frame | SHA256 |
|---|---|
| first | `f74c5e8cfc7e6ccc45d98a97a8240dbb25df828926548c7cfc8b80baea71da37` |
| repeat | `bf441968931aef5474c9719c915ca10689db059e2866391356ce60e67380c989` |
| east `(24,38)` | `78e0ee10d018debc85b26fb9a20bd5ce8d7b202804c0c86072c8f699ad2be721` |
| back `(22,38)` | `b8b23b455cc18543c5292adc81f5a512287f498d1512594074f08bfb904e4315` |

Each first/repeat frame has14 known land/7 known sea cells;56 opaque base,41 keyed shaded,2 keyed unlit Fungus,26 coast-mapped and28 water submissions. Native clear contributes523264 stores; terrain/unknown paint contributes104352 first and104353 repeat. Total stores are627616/627617. First/repeat differ because geometry/cache history is observable; do not reset cache between them. East/back execute retained-backing self-scroll, strip fills and unknown lines, with no newly painted known material. Complete native ordered stores also verify the copy/fill behavior.

## Smallest Unity Survey profile

For an ordinary positive viewer, rules bit`0x100` allows terrain for unexplored cells. With editor bit`0x80` clear, faction revelation bit`0x200` clear and the viewer exploration bit absent, `470008..47002c` applies:

- selected items `&= ~0x20` (clear Fungus only);
- center moisture=0 and rockiness=0;
- local style/blank override=1;
- full-light/color flag=0;
- landmark metadata word OR`0x200000` (render metadata, not exploration).

This preserves band, contour, geometry/cache and live neighbors. It does not create ordinary knowledge or a lower-console terrain card. Ordinary known/revealed cells retain moisture/rockiness and normal feature eligibility. With survey absent, an unexplored ordinary tile emits only the four unknown boundary lines.

Unknown survey land uses the arid base unless another earlier base replacement applies. Survey does **not** erase every selected improvement: a remembered Farm can still select a Farm base crop. The remembered-items control uses live `0x208024` and remembered `0x208024`; after masking it selects `0x208004`, emits four Farm base triangles and no Forest/Fungus/Road/sprite. Its image differs from empty remembered survey. The blank override skips landmark art, Forest and Fungus continuations (`47108a`, `471864`, `47195c`). The late exploration gate (`471f63..471fab`) skips remaining resource/improvement stages for the unknown tile. Early grid and coast paths can still run. An unknown land tile beside sea retains two coast sectors in the control; survey grid preference emits the two ordinary north grid edges.

For land lighting, full-light flag0 bypasses `47d330` and supplies global `6a9324` (`470a54`), **value2 in these controls**, rather than forcing vertex light0. Do not conflate moisture/rock suppression with flat geometry or zero lighting.

## What wrapper `0x41` actually does

It selects alternate-row indexed raster code, not a different palette or CLUT. Full wrapper `0xffffffff` delegates to the ordinary shader. In the alternate path, low-byte65 is the constant fill color and absolute destination Y parity controls rows: even Y writes index65; odd Y retains textured sampling. The normal common palette still applies.

There is an additional exact quirk in alternate opaque `63dcc0`: odd-row source fetch occurs before source-pointer advancement (`63e23e` before `63e244`). The first texel is repeated, then each destination pixel samples the preceding pixel's UV. Lighting phase still advances normally. The independent checker models this; simply using ordinary sampling on odd rows fails.

Keyed alternate rock keeps normal key9 rejection; coast alternate writes65 only where mask mapping is nonzero, preserving zero-mapped destination pixels. Its odd rows retain ordinary coast mapping/shading. The shelf survey control emits four water quads with wrapper`0x41`; the same indexed profile reproduces its full pixels. Do not indiscriminately paint black over polygon transparency.

Display option `959f9c&1` can independently select alternate rendering for an **already explored** tile through the current-observation search (`4700d3` onward). In the empty-roster known-display1 control, moisture2/rock2 and style0 remain, yet both base and rock use`0x41`. Unknown survey output is unchanged with that option. This proves alternate rendering and survey admission are distinct state decisions; this bundle does not claim to exhaust the complete current-observation search over Units, Bases and sensors.

## Reproduce verification

From the repository root (or frozen bundle root) with the user's installed assets:

```
python tmp/reference/check_partial21_source_contract.py --verify-only
python tmp/reference/check_partial21_material_pixels.py --verify-only
```

The15 controls verify2342148 native stores and7848960 complete indexed pixels.363 material primitives regenerate159489 exact ordered material writes from owned PCX crops, vertices/UVs/values and masks. Line, clear, fill and sprite writes also match exact ordered native stores. Self-scroll verifies the complete offset/value set and composed pixels because native CPU word-copy order differs from the row-major reference. All native store streams retain their own original ordered hashes.217 atlas sources are independently audited against owned art. Captured commands are verification expectations, never production renderer inputs.
