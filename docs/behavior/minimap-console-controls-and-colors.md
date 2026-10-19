# Minimap console controls and coarse overview colors

The pinned original `terran.exe` console constructor establishes **two** bottom-right map buttons: **World Map** and **Detail Map**. Their artwork matches the port's existing two atlas columns. **The reference's four-button row is not established by this evidence.** Do not turn the installed Physical/Climate/Rainfall/Political label strings into four supposedly native console commands without tracing their binding.

This bounded handoff provides two native button constructions, six actual command/refresh executions, and 28 full original coarse tile-renderer calls. See [machine-readable cases](minimap-console-controls-and-colors-cases.json) and the [private source map](../re-smac-minimap-console-controls-and-colors.md). No production source changed.

## Native console geometry and artwork

Coordinates are console-local at 1024×768; add511 to y for screen coordinates. All rectangles below are half-open.

| Region/control | Rectangle | Command |
|---|---|---|
| Minimap region | `[780,88,1019,207)` | — |
| Year/treasury information region | `[785,214,901,246)` | — |
| World Map | `[904,214,958,246)` | 1006 / `0x3ee` |
| Detail Map | `[960,214,1014,246)` | 1007 / `0x3ef` |

Each button is54×32 native pixels. The original extraction uses `interface.pcx`, transparent palette index9:

| Button | First sprite | Second sprite | Third sprite |
|---|---|---|---|
| World Map | `[532,143,586,175)` | `[532,176,586,208)` | `[532,209,586,241)` |
| Detail Map | `[587,143,641,175)` | `[587,176,641,208)` | `[587,209,641,241)` |

These three sprites are wired into the native button's three state-sprite fields in that order. The probe does not repaint the common button widget to independently re-establish every hover/press transition. The installed atlas itself labels these two columns “Map Mode Buttons”; that caption is artwork, not executable command evidence. Actual loaded tooltip strings independently identify World Map and Detail Map.

## What the two commands do

World Map selects command1006, retains detail-choice0, hides the detail view, and shows the overview window. Detail Map selects command1007, retains detail-choice1, shows the detail view, and hides the overview window. Selecting the same command again preserves that choice. The native refresh helper restores the same selection and visibility pair from the retained choice.

**Detail Map replaces the overview with the detail view; it does not leave an empty black panel.** The port's current `minimap.hide()` action only implements part of that transition. These two commands are not shown to select physical/rainfall/political palettes. Mode save/load persistence, complete detail-view camera/navigation, and enlarged-map side effects are outside these receipts.

## Coarse overview palette decisions

The following cases execute the complete original tile renderer at zoom−14, with explicit coarse-overview flag`0x80000000`. Its final polygon-fill call is an observation sink. They establish color and visibility decisions, not a complete minimap screenshot or pixel-edge rasterization. A null view argument and explicit target/anchor isolate the decisions; whole minimap sizing and resampling are not newly validated.

Known sea tiles, altitude bands0..2, use palette index190, RGB **(14,37,75)**. In the tested land political and rainfall variants, claimed sea still uses this sea color.

For ordinary known land in the physical path, index=`2*(14-altitudeBand)`:

| Land band | Index | RGB |
|---|---|---|
| 3 | 22 | `(80,65,51)` |
| 4 | 20 | `(93,76,59)` |
| 5 | 18 | `(106,87,67)` |
| 6 | 16 | `(120,98,74)` |
| 7 | 14 | `(133,109,82)` |

Adding renderer flag`0x10` selects the tested land moisture palette: index=`149+2*moistureBits`.

| Moisture bits | Index | RGB |
|---|---|---|
| 0 | 149 | `(212,208,116)` |
| 1 | 151 | `(152,176,76)` |
| 2 | 153 | `(88,140,44)` |

A raw value3 diagnostic also executes and selects index155; this is not a claim that ordinary climate producers generate a fourth rainfall category. **A console command selecting this flag has not been established. Climate scan semantics remain unresolved.**

## Political territory and knowledge

Adding renderer flag`0x10000` invokes the original territory resolver for admitted known/revealed land. The tested path reads the retained tile's signed claim byte, not unit ownership, nearest current base by an invented rule, or a reconstructed history map.

For viewer3:

- Own positive claim3 is colored with that slot's primary map index.
- Unclaimed255 or claim0 falls back to physical land color.
- Foreign claim2 without the native recognition mask falls back to physical land color.
- Foreign diplomacy mask8 alone, or`0x08000000` alone, is insufficient. The resolver requires both, mask`0x08000008`, in the ordinary noneditor path.
- Editor mode bypasses that foreign-claim recognition filter for the admitted known tile.

The claim producer and recognition behavior are reused from [land territory rebuilding](land-territory-producer.md); these controls do not create claims or diplomatic recognition.

The actual default palette-table initialization yields these slot colors:

| Slot | Index | RGB |
|---|---|---|
| 0 | 249 | `(255,0,0)` |
| 1 | 241 | `(0,235,0)` |
| 2 | 252 | `(0,0,255)` |
| 3 | 255 | `(255,255,255)` |
| 4 | 251 | `(255,255,0)` |
| 5 | 65 | `(0,0,0)` |
| 6 | 102 | `(235,92,0)` |
| 7 | 235 | `(164,176,232)` |

These are original initialized map-slot entries at gamma100, not vehicle material tints or faction-name text colors. Slot0's entry existing does not make a raw claim0 politically owned; the native resolver rejects nonpositive claims. Preserve the engine's established slot/faction assignment; this receipt does not establish color remapping for mods or reordered factions.

With [Standard rules](standard-current-rules.md), initial survey bit`0x0100` is clear. An unknown tile whose exploration byte lacks the viewer bit submits **no polygon**, even when its full terrain and claim are retained in the imported Planet world. The caller's existing backing remains; this does not by itself prove that backing is black.

Turning on initial survey admits an unknown tile's physical geography, but the tested political path still falls back to physical color for that unknown own-claim tile. The faction all-map-revelation status`0x0200` admits the tested unknown tile and its own political color. Revelation alone does not bypass the separate foreign recognition check: an unrecognized foreign claim under revelation still falls back to physical color. Terrain exploration/revelation and foreign-territory recognition must remain separate.

## Port boundaries and remaining unknowns

The two verified controls can be wired to an overview/detail choice now, with an actual detail-view replacement and retained selection. The color decisions can be implemented as renderer policies using existing claims, exploration, rule, and recognition state. **Treat any new political/rainfall UI selector as a port choice until its original command binding is established.**

Still unresolved: the four-button reference row's identity/executable/version, its exact glyphs/hit rectangles/commands, Climate scan, complete minimap resampling and painted output, detail camera/viewport behavior, historical sampling, and mode save/load lifecycle. Merely finding four installed scan labels does not close those gaps. No full-map history or side effects are inferred.
