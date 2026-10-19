# Lower console: unit cards, roster, and terrain preview

The original Scout selected card displays `(1-1-1)` and `Moves: 1`. Its name is **Arial bold −12 pixels**; its statistics, morale, home, movement, and order rows are **Arial regular −12 pixels**. Terrain rows and roster statistics also use Arial regular −12 pixels. These are direct receipts from the respective callers, independently of the faction-summary font contract.

Machine-readable layouts, font records, and 16 formatting cases are in `lower-console-unit-card-cases.json`. Addresses and evidence boundaries are in `../re-smac-lower-console-unit-card.md`.

## Coordinate convention

All coordinates here are console-local for the native 1024×768 layout. Add **511 to y** to obtain screen coordinates. Rectangles are half-open. Do not treat these native pixels as CSS points.

| Region | Outer rectangle | Content/clip rectangle |
|---|---|---|
| Selected card | `[8,60,136,248)` | `[14,66,130,242)` |
| Terrain card | `[140,60,241,189)` | `[146,66,235,183)` |
| Lower roster | `[139,192,764,248)` | First cell `[143,196,208,244)` |

## Model placement and overlays

The model is rendered into a 256×256 work surface around its internal pivot, then translated to the target. **There is no additional fit of that entire transparent work surface into the card or cell.** Fitting the surface into a 100×56 or 57×40 widget introduces another unwanted shrink.

| Caller | Body anchor | Body zoom | Body flags | Separate overlay anchor / zoom / flags |
|---|---|---|---|---|
| Selected card | `(34,66)` | −4 | `0x48` | `(14,66)` / 0 / `0x1800` |
| First roster cell | `(156,197)` | −8 | `0x400` | `(145,197)` / 0 / `0x1800` |

The body passes suppress their overlays. The overlay pass suppresses the body and uses zoom 0. Keep the order badge and health presentation separate from model scaling. The Scout chassis dispatcher preserves these body anchors.

The two complete native Scout idle draws, with facing 0 and animation counter 0, give:

| Measurement | Selected | Roster |
|---|---|---|
| Final model scale | `0.06562500447034836` | `0.04374999925494194` |
| Work-surface wrapper anchor | `(71,103)` | `(181,222)` |
| Work-surface copy offset | `(−57,−25)` | `(53,94)` |
| Nontransparent work-surface bounds | `[118,102,133,135)` | `[121,110,132,133)` |
| Translated visible bounds | `[61,77,76,110)` | `[174,204,185,227)` |
| Visible pixel count | 212 | 113 |

The copy offset is `wrapperAnchor − (128,128)`. Bounds above describe this explicit idle fixture, not every facing or animation frame. Each draw executes all 59 composed objects and 65 kernels. Translation is calculated from the observed wrapper anchor; final target blitting is covered by the existing `scout-target-copy.md` contract.

## Text and baselines

Current Windows Arial metrics are height 15, ascent 12, descent 3, internal leading 3. The native text API places the baseline **9 pixels below logical y**, while the caller advances ordinary rows by **13 pixels**. Font creation requests weight 0 for regular (resolved metric weight 400) and weight 700 for bold.

| Selected Scout row | Example | x | Baseline y | Weight |
|---|---|---|---|---|
| Name | `Scout Patrol` | 37 | 139 | 700 |
| Statistics | `(1-1-1)` | 53 | 152 | regular |
| Morale | `Very Green (+)` | 14 | 165 | regular |
| Home | `Independent` | 14 | 178 | regular |
| Movement | `Moves: 1` | 14 | 191 | regular |
| Order | `Hold` | 14 | 204 | regular |

Name and statistics use the native centered-text path; the recorded x positions depend on the string width. Other rows begin at x=14. The neutral controlled unit has no home base; its morale text is an observed fixture result, not a promise about a naturally started Scout.

Ordinary selected and terrain text uses palette index 152, RGB `(116,156,56)`, without a shadow. First roster-cell statistics are bare `1-1-1`, centered at x=161, baseline y=242, regular font, palette index 154, RGB `(235,235,235)`.

## Statistics and remaining movement

The native shared formatter resolves design weapon, armor, and movement values. The selected card wraps that result in parentheses; the roster does not. Movement in the statistics is an integer quotient of resolved design movement allowance by the movement quantum, not a decimal or current remaining movement. A reactor above 1 adds `*N` after the movement number. Ability-dependent decorations also exist; the cases preserve observed strings without claiming a complete ability-formatting grammar.

| Fixture | Selected statistics | Movement row |
|---|---|---|
| Scout fresh, quantum 3 | `(1-1-1)` | `Moves: 1` |
| Scout spent 1 quantum | `(1-1-1)` | `Moves: 0 2/3` |
| Scout spent 2 quanta | `(1-1-1)` | `Moves: 0 1/3` |
| Scout spent 3 or 4 quanta | `(1-1-1)` | `ALREADY MOVED` |
| Unity Rover fresh | `(1-1-2)` | `Moves: 2` |
| Unity Rover spent 1 quantum | `(1-1-2)` | `Moves: 1 2/3` |
| Scout, quantum 5, spent 1 | `(1-1-1)` | `Moves: 0 4/5` |
| Scout, quantum 4, spent 2 | `(1-1-1)` | `Moves: 0 2/4` |
| Scout clone, reactor 2 | `(1-1-1*2)` | `Moves: 1` |
| Mind Worms | `(?-?-1)` | `Moves: 1` |

For the owned-unit movement row, remaining quanta come from the native per-unit allowance minus the unsigned spent byte, bounded to 0..999. Positive remaining movement formats as `Moves: ` plus integer quotient, followed by ` remainder/quantum` only when the remainder is nonzero. Keep the zero whole part and do not reduce fractions. At zero or below, use installed label `ALREADY MOVED`, in red `(255,0,0)`, rather than `Moves: 0`.

The `?` values are valid psi statistics for Mind Worms; they are incorrect defaults for the ordinary Scout and Rover.

### Elite and damaged units: tuple speed stays nominal

The tuple uses resolved **design** movement allowance, before the live unit's Elite morale and damage modifiers. Do not build the tuple from a fresh per-unit allowance. Ten additional complete original selected/roster caller runs discriminate these inputs; exact cases are in `lower-console-tuple-speed-cases.json`.

| Unit / effective morale / damage | Selected tuple | Roster tuple | Fresh movement row |
|---|---|---|---|
| Scout / 1 / 0 | `(1-1-1)` | `1-1-1` | `Moves: 1` |
| Scout / Elite 6 / 0 | `(1-1-1)` | `1-1-1` | `Moves: 2` |
| Scout / Elite 6 / 5 | `(1-1-1)` | `1-1-1` | `Moves: 1` |
| Rover / 1 / 0 | `(1-1-2)` | `1-1-2` | `Moves: 2` |
| Rover / Elite 6 / 0 | `(1-1-2)` | `1-1-2` | `Moves: 3` |
| Rover / 1 / 5 | `(1-1-2)` | `1-1-2` | `Moves: 1` |
| Rover / Elite 6 / 5 | `(1-1-2)` | `1-1-2` | `Moves: 2` |
| Rover / Elite 6 / 9 | `(1-1-2)` | `1-1-2` | `Moves: 1` |

Damage is the native unsigned damage byte, not a percentage. These controlled fixtures have spent=0 and quantum=3. The full selected and roster callers both run for every case; the parentheses difference is directly observed for Scout and Rover, including Elite and damaged variants.

## Terrain preview and first text row

The terrain caller uses anchor **(148,67)**, zoom 0, preview scale ratio **85/100**, and clip `[146,66,235,183)`. It restores the global preview-scale override after this call. The first ordinary text logical y is **113**, giving baseline **122**. In this rectangle the caller computes logical y as `contentTop + floor(contentWidth/2) + 3`.

The controlled flat/arid land tile displays `Elev: 10m` at x=146, baseline 122, followed by `Flat & Arid` at baseline 135. Coordinates `(8 , 8)` appear at x=202, baseline 179. Preserve the text baseline independently of the rendered preview bounds. Resource-yield display mode is a separate path and is outside this fixture.

The subsequent [flat-preview extent case](lower-console-terrain-extent-case.json) executes the original terrain renderer with those exact console arguments through all four base triangle submissions. The resulting mesh is **80×40 pixels**, centered at **(188,87)**, with W/N/E/S vertices **(148,87), (188,67), (228,87), (188,107)**. The submitted triangles are C-W-N, C-N-E, C-E-S, C-S-W; their coordinate extrema are `(148,67)..(228,107)`. These are geometric endpoint coordinates, not a half-open painted-pixel bounding box. The target clip remains `[146,66,235,183)` and does not trim this flat mesh.

The native integer scale path computes `trunc(50*85/100)=42`, then `trunc((42+1)/4)=10`; diamond width/height are `8*10` / `4*10`. Do not scale a 96×48 diamond by a floating 0.85 and assume 81.6×40.8, or fit it into an 84×52 thumbnail. Raster-painted extents and complete preview overlays remain deferred; this fixture observes raster submissions rather than executing the terrain sampler. The ordinary main-map terrain contract is unchanged.

## Aircraft boundary

The installed Unity Scout Chopper produces `(<1>-1-8, SAM)`, `Moves: 8`, then a separate `Fuel: ` and green `8/8` row at baseline 204. Spending one quantum gives `Moves: 7 2/3` and fuel `7/8`; spending three gives `Moves: 7` and `7/8`. The order row shifts down to baseline 217.

A declared Scout design clone with Needlejet chassis produces `(1-1-10)`, `Moves: 10`, and green `20/20` in this loaded-rule fixture. It is not a naturally created prototype. The exhausted Chopper fixture gives `ALREADY MOVED` and yellow `8/8`; therefore do not derive the fuel row by simply reusing the movement fraction or extrapolate a fuel lifecycle from these receipts. Full aircraft fuel/turn bookkeeping remains outside this bounded handoff.

## Validation limits

The selected-card routine and terrain/roster routine execute original x86 formatting and layout code with loaded installed rules, labels, predefined designs, palette, and actual current Windows font metrics. Body and terrain rendering are observation sinks in those caller runs. Separate body runs execute the full original Scout draw. These are not live original-game screenshots or historical Windows raster comparisons. The existing base-name and terrain geometry contracts are unchanged.
