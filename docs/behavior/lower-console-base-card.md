# Selected Base card and first Base roster cell

This closes the selected Base portrait/body, population badge, selected-card name/production text and first roster Base cell for the fixed1024×768 stock profile. [13 native contexts](lower-console-base-card-cases.json). All coordinates below are **console-local**; console origin is(0,511), so add511 to Y for full-screen positions. This reuses the established console construction and font services, with actual loaded University Base art; no production/owned assets changed.

## Two render passes, distinct anchors

The selected Base card draws its Base artwork at **(34,66), zoom−4,flags0x40**, then population separately at **(34,66),zoom0,flags0x1800**. The native artwork scale is12/16: its original100×75 logical frame becomes75×56 by native truncation. The call anchor is the original sprite-frame origin, not the center of a68-pixel square, a terrain-mesh center, or the cropped ink center. Loaded sprite offsets and exact native scaling determine its destination. The label center and sprite centering can differ by half a pixel because the renderer uses integer placement.

The first Base roster cell rectangle is[143,196,208,244). Its artwork call is **(156,197),zoom−8,flags0x40**, scale8/16, logical frame50×37. Its population call is independently **(145,197),zoom0,flags0x1800**. Preserve this11-pixel X difference. The first roster clip is the cell rectangle; selected Base calls in this fixture use the full native bitmap clip. The roster builder also redraws the selected card earlier; the cases isolate its final first-cell passes.

## Original atlas and native rectangles

All artwork is from UNIV.PCX for original/former artwork owner3; key9. Native Base selection chooses distinct land/sea rows. Population1 uses size0 art;9 and10 select the same size class in the loaded standard rules, so their bodies match while their badge widths differ. These are measured choices, not an assumption that every integer population has a separate portrait.

| Context | Original crop[x,y,w,h] | Native scaled destination, exclusive end | Pixels changed on diagnostic index100, bounding box |
|---|---|---|---|
|selected_land_pop1|[1, 1, 100, 75]|[54, 78, 84, 106]|[55, 78, 84, 106]|
|roster_land_pop1|[1, 1, 100, 75]|[169, 205, 189, 224]|[170, 205, 189, 224]|
|selected_land_pop9|[203, 1, 100, 75]|[38, 72, 103, 115]|[38, 72, 103, 115]|
|roster_land_pop9|[203, 1, 100, 75]|[159, 201, 202, 230]|[159, 201, 202, 230]|
|selected_land_pop10|[203, 1, 100, 75]|[38, 72, 103, 115]|[38, 72, 103, 115]|
|roster_land_pop10|[203, 1, 100, 75]|[159, 201, 202, 230]|[159, 201, 202, 230]|
|selected_sea_pop1|[1, 229, 100, 75]|[47, 78, 103, 120]|[47, 78, 103, 120]|
|roster_sea_pop1|[1, 229, 100, 75]|[165, 205, 202, 233]|[165, 205, 202, 233]|
|selected_sea_pop9|[203, 229, 100, 75]|[34, 72, 106, 122]|[34, 72, 106, 122]|
|roster_sea_pop9|[203, 229, 100, 75]|[156, 201, 204, 234]|[156, 201, 204, 234]|
|selected_sea_pop10|[203, 229, 100, 75]|[34, 72, 106, 122]|[34, 72, 106, 122]|
|roster_sea_pop10|[203, 229, 100, 75]|[156, 201, 204, 234]|[156, 201, 204, 234]|

`native_destination` is captured inside the original scaled sprite helper before clipping. The changed-pixel box comes from actual native artwork rendering onto a diagnostic index100 background, before the population pass. It is not asserted as a background-independent opaque-coverage mask; a pixel equal to the background is not a change. All source data and image buffers remain private. The logical-frame origin, loaded retained offset/size, scale ratio and both observed rectangles are available separately in each case.

## Population badge and glyph ink

The native badge is anchored to its separate zoom0 pass, so it retains the same size in selected and roster views. Population1/9 outer boxes are14×16; population10 is23×16. Native layout measures the placeholders8/88 rather than stretching to the portrait. The text starts anchorX+3, baseline anchorY+14. Captured owned-empty fixtures draw a black frame and white digits without a new interior fill. Other occupancy/faction style branches retain their separate map-overlay limits.

Font is **Arial bold700, logical height−16**. The box/anchor/baseline are native; the glyph ink boxes below are separately measured using the actual font request on current Windows GDI. Historical Windows font raster parity is not claimed.

| Context | Native box | Text start/baseline | Current-GDI glyph ink, exclusive end |
|---|---|---|---|
|selected_land_pop1|[34, 66, 48, 82]|[37, 80]|[38, 68, 43, 80]|
|roster_land_pop1|[145, 197, 159, 213]|[148, 211]|[149, 199, 154, 211]|
|selected_land_pop9|[34, 66, 48, 82]|[37, 80]|[37, 68, 45, 80]|
|roster_land_pop9|[145, 197, 159, 213]|[148, 211]|[148, 199, 156, 211]|
|selected_land_pop10|[34, 66, 57, 82]|[37, 80]|[38, 68, 54, 80]|
|roster_land_pop10|[145, 197, 168, 213]|[148, 211]|[149, 199, 165, 211]|

Land and sea have the same badge geometry for a given population. The first roster cell contains artwork and population in these controls; it does not repeat the selected card's Base-name/production block.

## Base name, production and wrapping

The selected name uses **Arial bold700, height−12**. `New Austin` starts x40, baseline139. Production/progress uses **Arial regular, height−12**, and is centered/wrapped by the original text routines:

| Text | x | Baseline |
|---|---:|---:|
|New Austin|40|139|
|Producing: Scout [trailing space]|24|152|
|Patrol (4/10)|38|165|

The native predefined Scout Patrol production with4 minerals is a two-line production string here; `(4/10)` is embedded in that wrapped text. No separate progress bar was observed in these contexts.

The diagnostic24-character Base name `University Research City` wraps as `University Research ` at x14/baseline139, then `City` at x61/baseline152. Native predefined Isle of the Deep,74/80 minerals, follows as `Producing: Isle of the ` at x13/baseline165 and `Deep (74/80)` at x36/baseline178. Each wrapped row advances13 pixels; production begins after the last name row. These literal whitespace/layout results use the existing localized string table and current Windows metrics. Do not force a single-line label or fixed production baseline when the name wraps.

## Scope and reproduction

The independent supplement adds two full private indexed canvas hashes immediately after native retained-art drawing, land/pop1 at12/16 and8/16. These satisfy a captured-output reference; exact portable downsample stepping remains open. Retained offsets and dimensions still scale separately, rather than resampling the full100×75 logical canvas.

The native plain-space wrap routine tests chunks ending before the next separator, including a leading separator on later chunks, against the remaining116-pixel width; equality fits. Once the next word overflows, the painted substring retains the previous trailing space. Native measurements are `University`56 plus ` Research`58=114, followed by rejecting ` City`24; the painted bold row is117 pixels with its trailing space. Production chunks total59+23+13+20=115 before rejecting ` Deep`33; its painted regular row is118 pixels. Centering uses that painted width, explaining x13 for the118-pixel row. These plain-label observations do not close markup or overlong-word edge cases.

The cases also publish95 numeric ASCII advances32..126 for Arial height-12 regular and bold700 from the identical current-GDI font services. All9025 pairs per font matched the sum of individual advances; representative whole labels also match. No explicit kerning is applied by this measured service. These numeric fixtures support asset-free tests of the observed labels; installed-font checks and historical raster parity remain separate. The original13-context/85-assertion archive is unchanged; hashes, measurements and packaging are frozen separately in `tmp/reference/base-lighting-input-supplements-evidence-20261019`.

The original selected-card and full roster routines execute. Original University faction PCX extraction, native body art, population frame and text layout execute. Neutral facility/art-status queries are supplied; terrain preview raster is a declared sink. The actual loaded Base name/production/rules and current GDI font services are retained. Roster runtime/session admission is enabled explicitly; its producer and selection/data binding remain separate. No new click behavior is claimed.

Reproduce private probe_console_base_cards.py, then finalize_console_base_cards.py. Native source coordinates and expected numeric cases are public; original assets and buffers remain outside production. Other factions, facility art variants, occupied/foreign styles, other resolutions, dynamic production queues and whole-console screenshot parity are outside this finite handoff. Broad RE remains paused.
