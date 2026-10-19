# Complete indexed viewport comparison checkpoint

`tools/reference/capture_native_viewport.py` drives production `ViewGeometry` and `RenderPrepared` from the owned installed raw Planet records and decoded PCX art. It emits first/repeat/east/back indexed frames, using explicit allocation-low16 variation and initial backdrop inputs. No recorded query coordinates or draw lists are injected. Art-derived outputs belong in an external private directory.

Pass `--reference-receipt` to require complete frame hashes. With `--reference-frame-directory`, mismatch diagnostics also validate the private original bytes against the receipt and report differing pixel counts/coordinates. A mismatch is a failing command, not an accepted tolerance. The expected original receipt is still being finalized in the separate reverse-engineering thread.

The initial original diagnostic uses viewer0, flags0x10621, known raw Planet, empty rosters, camera22,38 then24,38/back, zoom0, allocation low16=0xbf60, clear index65. Original viewer0 reads its remembered-improvement word at the landmark offset; `--observer0-alias` reproduces that diagnostic input explicitly. It is not a normal playable-faction view and must not change gameplay observer semantics. A valid-faction control remains required.

At this checkpoint, ASan/UBSan production captures return normally. First/repeat/back are internally identical. Against the original diagnostic, first/repeat/back each differ at2312 of523264 indices; east differs at2319. Original shadow LUT bytes match. Last-native-writer attribution for the2312 first-frame differences identifies1686 water-depth stores at63ef70/caller470ff7 and626 coastal-mask stores at63e880/caller471d98. The water/coast caller inputs and mutable raster state are under investigation; no complete viewport parity is claimed. Earlier486 isolated tile frames and geometry-schedule proofs remain narrower evidence.

## Closed diagnostic differences

The production fix keeps observer-selected sea-Fungus flags separate from the live River cell input, and stops applying ordinary terrain shade caps to `CoastMapped` depth interpolation. A direct replay of the first differing original coastal root has454 stores: applying[-4,4] caps changes321 pixels; omitting those caps reproduces every original pixel. The matching water-depth root already agreed. These focused replays diagnose the generic rule; the complete production comparison still generates all its own commands from raw map/camera inputs.

With both fixes, all four complete diagnostic frame hashes match exactly under ASan/UBSan. First/repeat/back are `b6aaa64aad965a4df1f0be3fc02dfc022fb898bbbcbbdbfab45a8ae7235d4f33`; east is `c34ca964f934739af1792006244d2b82e6d6a0aff5471589cc98810ee9e2c736`. The first production paint has1,347,720 stores, equal to original stores excluding its523,264-byte clear. All60 prior water/coast/sea-Fungus indexed frames and486 original tile-frame controls remain exact. Cached build,15 affected native groups with memorydebug and targeted correctness analysis of both changed production translation units pass. Added live-renderer coverage proves that remembered absence suppresses live sea Fungus and remembered presence survives live absence.

At this intermediate checkpoint, the original numeric receipt and a separate valid-faction fullframe control were still being finalized; the completed comparison is recorded below. The viewer0 alias frame must remain labeled diagnostic; it does not establish a normal playable-faction viewport, full game UI, arbitrary cameras, or hardware presentation parity.

## Valid faction and checked numeric receipt

The separate valid display-faction3/all-map-revealed controls also match all four complete frames and every paint-store count under ASan/UBSan. These controls select live item words, with resource rules/preferences0, no rosters, default relief and the same explicit variation allocation. First/repeat/back hash is `6f0767ae1eb7cc6c3d4658e9bcfd6ab0166295b32f3a31973f219859d4b65cd0`; east is `8799dd6de0918c8f3883beed236489ecb8cb8be80fe37673553b743e1a98a0a4`.

The public numeric receipt `viewport-indexed-pixel-cases.json` records all eight original frame hashes, paint-store counts, context and source-asset fingerprints. It is reduced from the complete native v2 observation file, whose exact source SHA256 is retained. No original pixels, map records or artwork are embedded. The probe verifies that receipt by default and checks observer/backdrop/resource settings, asset hashes and the shadow LUT before accepting a frame.

Run from the checkout with purchased assets and a private output folder:

```
python3 tools/reference/capture_native_viewport.py --asset-directory OWNED --output-directory PRIVATE --variation-low16 0xbf60 --initial-index 65 --viewer 3 --cxx clang++ --sanitize
```

For the separate viewer0 diagnostic, use viewer0 and `--observer0-alias`. This is exact indexed terrain/feature pixel parity for the recorded camera sequence and flags, not full game-screen parity. The original runs use declared generic memory/window services and empty rosters; partial fog, city/unit art, arbitrary camera histories, natural startup and parent-window backing remain separate targets. The RGB comparison image uses the captured original palette only for presentation; the test compares raw index bytes without tolerance or resampling.
