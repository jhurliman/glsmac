# Private ordinary terrain projection and lighting

Pinned terran.exe SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Public [contract](behavior/ordinary-terrain-projection.md), [reference](behavior/terrain_geometry_reference.py), [cases](behavior/ordinary-terrain-projection-cases.json). Freeze `tmp/reference/ordinary-terrain-projection-evidence-20261019`.

## Reuse and actual execution

Reuse existing raw Planet loader, climate semantic/rules, contour prose correction, and water base/depth-overlay receipts. No climate/timer/blink grid is rerun. `probe_terrain_geometry.py` composes `probe_planet_map_import.py` before `changes=[]`: actual complete5c0b00 loader, installed Planet hash `24b1a6b018f0aaddde0ecc5712cc302359c872228b0342f0f30b9004670cdb30`, native allocation/header/tile loading, existing post-load display-reset sink. No original assets are written or copied into Git.

An explicit0x22000-byte view backing has a supplied generic-window vtable offset0, width at+4c4=1024 and signed native height+4c8=−511. Zoom+1dd98=0, camera+1dd9c/+1dda0 and visibility are supplied. Scale override6b0114=0. Complete47dc00 allocates/initializes the12-byte-per-tile render cache. Complete46f3b0 computes view geometry and25/8 height step. Cached zoom0 means its unrelated font-refresh arm is not entered; no projection/height/lighting output is supplied.

Complete47cfc0 generates196 slope entries through original integer sqrt642ee0. Complete47ca10,47d0f0,47d330 and46f930 return normally. Original46fd00 runs from entry to explicit pre-instruction boundary47102f, after base geometry and before landmark/feature art continuation. It is not called a full renderer return. Raster functions63dcc0,63d6b0,63ef70 and635af0 are observation sinks with original callee cleanup6/6/7/5 arguments. They capture actual vertex and scalar arrays. No contour/height/slope/texture-choice return is substituted.

`check_terrain_geometry.py` checks16 land contexts against the independent public named-field reference, including exact native camera-origin derivation, helper anchor, all heights/light values and all primary triangle arrays. Three raw Planet controls are35,5 coast;43,9 slope;25,21 flat. Visibility byte is explicitly set for observer faction0; terrain/header inputs retain actual loader values. Selected neighborhoods exclude height-deformation bitsb03 within radius2, covering the target and its height contributors. Expected lighting uses the entire loaded band map: the public neighborhood excerpt is not advertised as a complete lighting stencil. Existing climate-preparation receipt remains separate: these are raw-loader terrain controls, not newly certified first-session worlds.

`check_terrain_water_geometry.py` checks two complete renderer-entry admissions, contour20/37, at zoom0. Complete46ebc0 is observed at entry and return470ecc for32 samples total; geometry is compared to the public plane reference. Prior supplied-scale timing grids are not rerun. Frozen final land/water JSON both pin the same final public reference SHA. Earlier exploratory/v2 logs are mutable development and not accepted evidence.

## Geometry source map

46f3b0:12-pixel unit at+1ddc8, fullwidth+1ddb4=96, fullheight+1ddb8=48, horizontal step+1ddbc=48, vertical step+1ddc0=24, offsets+1ddac/b0, spans+1ddcc/d0 and expanded spans+d4/d8, first coordinates+1dda4/a8. 46f3c3..46f450 uses zoom/global scale. 46f5ee..46f6b3 clamps/wraps camera-derived first coordinates and fixes parity. 46f74d sets height step+1dd6c=25;959f9c bit10000 selects8. This task exercises that display preference directly; its UI toggle producer is not newly replayed. 46f795..46f855 invalidates cached geometry on height-step/zoom changes.

46f930 is the coordinate-to-anchor helper. It chooses the visible wrapped copy (with a separate nearest-copy offscreen path), then x=`offsetX+(relativeX−1)*horizontalStep`. It queries corner heights4,3,2,1; y=`offsetY+(relativeY−1)*verticalStep−SAR(sum,2)`. 46fd00's46fe6f..46fecf computes the same mean and adds it back to incoming y.47070d..470729 then adds2*halfwidth-scale and1*halfwidth-scale, producing the flat center before individual displacement. At zoom0 these additions are48 and24.

4707ab..4708a4 builds12 vertices using6a8ef0/6a8f20: four triangle fans with x offsets `(0,-2,0),(0,0,2),(0,2,0),(0,0,-2)` times24 and y offsets `(0,0,-2),(0,-2,0),(0,0,2),(0,2,0)` times12. 6a8f50 assigns semantic vertex selectors `[0,1,2,0,2,3,0,3,4,0,4,1]`. 4709e0..470a1b subtracts47ca10 heights from each y;470a25..470a49 fills shade arrays via47d330 when the admitted raster variant permits it. Four primary63dcc0 submissions carry3 vertices/shade values each. Rockiness can add63d6b0 with identical triangles; these are separate texture layers.

## Height/cache source map

47ca10(x,y,vertex,this=view) validates center and returns0 for sea band<3 or absent cache. Cache atview+4 uses12-byte records: shade bytes0..3, height bytes4..8 (C,W,N,E,S), status byte9 and four packed slope nibbles at+a. FF is cold/sentinel. Cached heights return sign extended.

Center path47cb58..47cc5b uses retained band, scaled height step and optional landmark deformation. Ordinary formula atzoom0 is25*(band−3). Corner path47cc5e..47cfb1 uses4 offsets per vertex in6a8e70/6a8eb0, horizontal topology9b8b14bit1, vertical bounds9b8afc. Any invalid/sea contributor takes47cd57: average height0, slope total12 and boundary flag1. Otherwise complete center queries are summed/SAR2. Height and slope nibble are published into corresponding slots of all valid contributor caches. The slope nibble is `(sumBands−4*ownBand+4)&f`, or4 at a boundary. Shared seam controls verify both height and shade equality.

Landmark height branches47cac7..47cb58 use flags2,1,200,100,800 with shared detail and6a8654 mapping, and profiles7f0a8c/8134e8/813474/7f0990/813400. Profile generation, their special signed deformation/clamps, and5a9f30 lifetime are explicitly not closed here. Existing5a9dc0 climate invalidation remains authoritative; do not assume the ordinary stateless formula replaces landmark shapes.

## Lighting source map

47cfc0 fills4×49 entries beginning8135a0. Its effective indices are a,b=−3..3; face orientation rotates `(a,b)` by90-degree steps. Each oriented component is multiplied100. Native642ee0 integer sqrt normalizes; signed division computes−3*(A+B)/sqrt with truncation toward zero. Zero norm gives0. Consumer bases813598/8135c9/8135fa/81362b are8 bytes before each generated block; slope nibble1 maps to generated−3, nibble4→0, nibble7→3. All196 entries compare to the independently written formula, not copied bytes.

47d0f0 reads slope nibbles for adjacent corners, chooses the face-specific table, caches signed value+128 in a shade byte and returns signed shade. It is a discrete band-slope lookup, not a normal inferred from current pixel vertices. 47d330 vertex0 queries four face values/SAR2; outer vertices average8 incident face contributions/SAR3. Optional landmark shade profiles6a89f4/6a8b30/813668 are separate branches and excluded. Raster entry receives signed per-vertex values; ordinary land sets6b5564/68 to−4/+4. The final pixel interpretation of those bounds, palette/CLUT and interpolation is not claimed by the geometry reference.

## Sea boundary

470d8c..470e2e constructs four sector quads by averaging diamond edges. 470e54 onward calls46ebc0(x,y,sector,mode) for4×4 samples. Retained depth table6a91c0 converts sampler results into final depth scalars; sampler itself is not physical land height. 470f20..470f64 admits the fixed overlay if any depth≥16 and subtracts16 for its input. Full caller controls produce the same576,288-centered plane at contours20/37, depth25/11,8/4 raster submissions. Their32 actual sampler returns are retained. Prior texture identity/crop and timer-negative findings are reused; no new water animation/pixel claim.

## Validation and remaining renderer work

Final check logs cover18 geometry contexts and196 generated light entries. The public ordinary model uses named fields and integer formulas, no addresses or native bytes. Accepted snapshots include source, selected listings, results, public docs/model/cases and hashes of reused immutable manifests. Only installed asset hashes are saved; actual Planet/PCX bytes stay installed/private. No new original raster image is generated or committed.

Open dependencies: landmark height/shade profiles; full water-depth semantic translation/coastal depth; texture/UV/shore/fungus/rockiness composition; palette/raster interpolation and coverage; cross-tile occlusion/draw order; other zoom/global scale and full camera/GUI construction. General invalid/parity-corrupt input, arbitrary high slope-nibble jumps and tiny-map centering are outside the admitted reference. This is a renderer integration contract for ordinary retained terrain, not whole-scene or natural first-session parity. Broad RE remains paused; no production changes or messages to other chats.
