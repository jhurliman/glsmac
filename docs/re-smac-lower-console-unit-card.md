# Source audit: lower-console unit card and terrain/roster callers

Pinned `terran.exe` SHA-256: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Public handoff: `behavior/lower-console-unit-card.md` and `behavior/lower-console-unit-card-cases.json`.

## Source map

| Original entry/path | Evidence |
|---|---|
| `4c9350` | Console Arial regular/italic/bold font objects; actual CreateFontIndirectA and GetTextMetricsA receipts |
| `4660d0..4668ab` | Native console geometry construction prefix for 1024×768 |
| `59e110`, `62e8b0`, `59dfc0` | Full installed rule, label, and predefined-design loading |
| `4c9880` | Full selected-card routine; original body calls observed at `56fd50`, actual text path through TextOutA |
| `4c9a05`, `4c9a25` | Selected body zoom −4 / flags 48; overlay zoom 0 / flags 1800 |
| `5932d0` | Shared weapon/armor/movement string formatter; called by selected card and roster |
| `4ca0dc..4ca2bb` | Owned remaining-movement formatting, quotient/remainder, clamp, exhausted-label branch |
| `4ca677` onward | Aircraft fuel row, native `5d9730`/`5d0590` helpers; only recorded cases claimed |
| `4cb9f0` | Full terrain/roster caller, including selected-card subcall, reaches normal return |
| `4cc023..4cc079` | Terrain render entry `46fd00`, clip/preview anchor/scale override |
| `4cc08f..4cc0a9`, ordinary path `4cc9fd` onward | Initial terrain logical text y, elevation and ordinary descriptive rows |
| `4ce236`, `4ce256` | Roster body zoom −8 / flags 400; separate normal-zoom overlays |
| `4ce3c3`, `4ce53b` | Shared stat formatting and roster centered text |
| `56fd50` | Actual Scout body dispatcher executed separately; `4d7860` entry observed; anchors unchanged |
| `4d7860`, `631370`, `631613` | Separate complete Scout rendering through all 59 objects/65 kernels, wrapper pivot observed, stop before target copy |
| `5f0e00` | Bitmap software clip at object+20, intersected with full bitmap bounds at +30 |
| `5f4b00` | Logical-y to baseline translation using ascent minus internal leading; actual API receipts |

## Fixtures and boundaries

Unit0 is explicit, at (8,8), owner/viewer 3, predefined design2, Hold order2, home −1, linked-stack fields −1. Per-case spent byte at unit+28h and movement quantum `9ba428` are supplied. Runtime design stride is 52 bytes; chassis is design+24h and reactor design+27h. Diagnostic designs15/16 copy the actually loaded Scout record and change only chassis/reactor. No natural turn, creation, or fuel lifecycle is claimed.

The selected-card runs replace `56fd50` and unrelated Base population rendering with observation sinks. They do not replace combat/stat/movement helper results. TextOutA and text measurement bridge to current Windows GDI receipts; no live screen is captured. Terrain/roster runs observe `46fd00` at entry instead of painting terrain. Separate dispatch runs execute `56fd50` and stop at its `4d7860` body helper. Separate full CVR runs use original VI/viptr00 assets and preexisting renderer harness, transparent index9, initial depthFFFF, facing0, animation counter0, renderer mode1; actual body flags/zooms are supplied. The existing full-render harness has controlled explicit catalog/unit backing and does not reload a naturally played campaign.

The visible translated bounding boxes are calculated from actual rendered indexed pixels and the actual wrapper anchor using the independently completed target-copy contract. The console caller and full body runs are separate compositions; this is not a screenshot of the entire lower console.

Full font requests: face Arial, height−12; regular LOGFONT weight0 resolves to metric400; bold700. No forced quality or character-spacing changes are added. Baseline delta9 and measured widths are current-host dependent; caller logical positions/13px row advance are original x86 decisions.

## Immutable evidence

`tmp/reference/lower-console-unit-card-evidence-20261019/manifest.json`

Manifest SHA-256: `01ec3756872a530e580e89e3f530ceda377d0cc0a573ceeb0295f2d4e57455e8`.

Bundle includes the four new probes, native receipt JSON/logs, source listings, immediate harness sources, finalizer/check script, and public semantic JSON. Existing nested harness dependencies remain in the working reference tree and prior frozen evidence bundles; this bundle is an audit snapshot, not a standalone redistributable emulator package. Original executable/assets are not copied into it.

All focused assertions passed: 16 full selected-card cases, one full terrain/roster call, two original dispatcher calls, two complete Scout body draws. No production source or preexisting frozen evidence was edited. No live original screenshot or historical antialiasing parity is claimed. The broad standing research goal remains paused.

## Nominal tuple speed supplement

The shared formatter `5932d0` calls design allowance `5d9400(design)`, divides by `9ba428`, and formats that quotient. It does not call live per-unit allowance `5d9530(unit,suppressElite)` for the tuple. The owned Moves path does use the latter helper. This distinction is now discriminated by ten full `4cb9f0` runs with native selected-card subcalls and roster draws: predefined designs2/11 × stored morale1/6 and damage0/5/9 as enumerated in `behavior/lower-console-tuple-speed-cases.json`. Effective morale is queried natively and asserted equal to supplied1/6; design allowance remains3/6 while live allowance changes with Elite and damage. Selected tuples retain parentheses; roster tuples are bare in all ten cases.

New probe: `tmp/reference/probe_console_tuple_allowance.py`. New immutable supplement: `tmp/reference/lower-console-tuple-speed-evidence-20261019/manifest.json`. It links the earlier bundle by manifest hash and leaves that bundle unchanged. Native per-unit fields: damage+10h, stored morale+24h, spent+28h. All ten full callers and helper comparisons passed; this adds no combat, promotion, or damage-producer claims.

## Console terrain extent supplement

`probe_console_terrain_extent.py` reuses the actual console fixture and target bitmap, installs clip `[146,66,235,183)`, and executes `46fd00(target,0,0,3,0,621h,8,8,148,67)` with global override85/100 through `47102f`. It observes the four original `63dcc0` triangle calls as sinks. All four clips and exact vertex arrays are asserted by `finalize_console_terrain_extent.py`; results are in `behavior/lower-console-terrain-extent-case.json`. At `46fe29..46fe68`, original integer arithmetic reduces the override to quarter-unit10; resulting mesh is80×40 with center188,87. This is the view-null console path, not substitution of a main-map camera/view. Uniform band3 backing is supplied as before. No terrain raster-painted pixel extent or full preview-overlay extent is claimed.

New immutable bundle: `tmp/reference/lower-console-terrain-extent-evidence-20261019/manifest.json`. Earlier frozen bundles and the general terrain geometry contract remain unchanged.
