# Expanded Farm, Sensor and Forest rendering contexts

> Semantic correction, revision2: original improvement names were wrong. Use the corrected raw-mask table and contexts in [the correction handoff](ordinary-map-improvement-semantic-correction.md). Frozen v1 coordinate evidence is retained; its old feature labels are superseded. Base/Scout rules are unchanged.

The corrected cases contain32 native rectangles: four Base fixtures and seven actual improvements across flat/uniform4/single-hill/coast worlds. All24 original raw rectangle cases remain identical after semantic renaming; the additions are true Farm and Sensor.96 rectangle-field assertions pass. These are loaded destination rectangles before clipping, not opaque ink bounds or composited pixel parity.

| Flat object | Native item mask | Caller anchor | Loaded offset | Loaded size | Destination rectangle, exclusive end |
|---|---|---|---|---|---|
|base|—|[528, 240]|[27, 17]|[41, 38]|[555, 257, 596, 295]|
|bunker|0x800|[528, 264]|[40, 18]|[50, 29]|[568, 282, 618, 311]|
|airbase|0x40000|[528, 264]|[26, 12]|[43, 26]|[554, 276, 597, 302]|
|mine|0x10|[528, 264]|[22, 1]|[57, 43]|[550, 265, 607, 308]|
|solar|0x40|[528, 264]|[25, 17]|[49, 23]|[553, 281, 602, 304]|
|condenser|0x400000|[528, 264]|[16, 2]|[63, 38]|[544, 266, 607, 304]|
|farm|0x8000|[528, 264]|[33, 28]|[14, 11]|[561, 292, 575, 303]|
|sensor|0x80000000|[528, 264]|[30, 1]|[27, 36]|[558, 265, 585, 301]|

For uniform4 shift both Y ends up25; single hill up6; coast up3. Base retains its ordinary projectorY−24 call anchor; the seven sampled improvement sprites use projectorY. Farm is0x8000; Forest is0x200000 and has a separately observed terrain quad, not one of these sprite rectangles. Its four-corner geometry is saved in the corrected feature contexts.

Base fixture remains University original/former artwork owner3,population3,size0,land,no facility variants. Original TER1 extraction and native sprite destination construction execute; actor/cache preparation is supplied. Repeated palette replacement is skipped only when the donor byte is absent from the entire source bitmap. Final clipping/pixel copy and other Base artwork variants remain outside scope.

The earlier failed extraction attempts and v1 frozen bundles remain unchanged. This revision preserves their history and valid raw coordinates while explicitly superseding every wrong improvement name. See the correction handoff for raw masks, atlas sources, new Farm/Forest controls and precise display-flag limits.

This is a separate finite supplement. Its loaded rectangles and display-flag contexts are in `ordinary-map-improvement-expanded-contexts-cases.json`. The canonical24-rectangle correction remains in `ordinary-map-loaded-sprite-rectangles-cases.json`.
