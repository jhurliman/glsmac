# Scout shadow renderer

The on-demand `CvrScoutRenderer::RenderShadow` entry point assembles the owned Infantry/reactor frames, generates the orthographic shadow camera from explicit scene/light/ground inputs, transforms simple and grouped origins, and walks the model delta streams. Color, normal and depth rejection do not affect the mask: repeated records stamp128 again. The returned surface owns its bytes and diagnostic counts.

`FlatScoutShadowScene` derives the checked default-light helper inputs from the existing native tilt/facing/scale producer. Inverse auxiliary-matrix cofactors and reciprocal determinant are stored to binary32 before the light transform. The camera then uses the portable64-significand arithmetic and explicit binary32 stores already used by the color renderer. See [camera evidence](behavior/scout-shadow-camera.md) and [raster evidence](behavior/scout-shadow-producer.md).

The admitted raster is the terminal, single-pixel, already-contained flags33 path. Block streams, alternate footprints, perspective and Speeder detail selection remain outside this entry point. Out-of-surface samples and signed coefficient/coordinate overflow reject. The indexed target blitter clips a1:1 copy and applies `lookup[existing target index]` for every nonzero mask sample. It validates buffers/clip and rejects source/target aliasing before changing caller storage. Palette-table production belongs to the caller; this change does not generalize the original x87 nearest-color precision edge.

The [initialized-light control](behavior/scout-initialized-light.md) confirms that ordinary shadows retain their independent fixed direction even when the SYSTEM map-light reader changes color lighting. The production initialized renderer now has a full-raster regression for both color controls and the identical222-pixel shadow. Ordinary map activation still needs destination palette-table production and indexed target composition; this confirmation alone does not add shadows to the live map. Shadow generation remains explicit, so ordinary color/card rendering keeps its existing cost. Complete populated-frame, animation and arbitrary-view parity remain open.

## Validation

- All76 affected CVR test groups pass, retaining prior color, cache, preview and teardown controls.
- The production scene producer and renderer reproduce all three complete256-square native masks by SHA256: facing2/222 pixels, facing3/262, translated raised ground/233. Each uses59 frames and65 kernels; the ordinary facing2 mask includes47,483 repeated stores.
- The production blitter applied to the original owning viewport's pre-shadow backing matches the complete target buffer and all222 native target records, SHA256 `b35d9ee026ce0d11843a627b9bce09d3a45edf0669cd74d590f131bb0f568ced`. The backing is test input; a captured mask is never renderer input.
- Six changed/affected production translation units pass correctness analysis. Three shadow groups also pass with seven affected translation units instrumented by ASan/UBSan; other linked objects are normal builds and leak detection is disabled.

Public fixtures retain scene inputs, formulas, semantic lookup values and digests. Owned model streams, original mask payloads and viewport backing remain outside the repository. The opt-in installed test reads `GLSMAC_SCOUT_ASSETS`; `GLSMAC_SHADOW_TARGET` and `GLSMAC_SHADOW_DUMP` support private original-target comparison. Synthetic camera, repeated-stamp, clipping, key semantics, overflow and aliasing tests run without original assets.
