# Native owner0 badge colors and morale text

RE-SMAC-NATIVE-DISPLAY-IDENTITY-001, 2026-10-18. Pinned stock SMAC.
[Source/asset mapping cases](native-display-identity-cases.json). This is a
static original-caller mapping with independently decoded installed samples;
no native raster pipeline or lifecycle controls were rerun.

**Native owner0 uses the red badge palette: primary index249, RGB255/0/0;
secondary index1, RGB128/0/0.** It does not borrow the human ecology-source
faction's flag, and owner0 is distinct from the port's controller slot0.

## Exact source recipe

The original shared badge builder samples `PALETTE.PCX`, not the ecology
source faction's PCX. For owner `f`, its primary sample is zero-based
`(580+8*f,23)` and secondary sample `(580+8*f,31)`. The original PCX load uses
index offset10. Owner0's file indices239/247 therefore become byte indices
249/1, respectively, including byte wrap on247+10. Resolve them through the
active game palette. Both are reserved constructor entries preserved by the
installed main palette load, as already checked in the
[palette-input receipt](faction-actor-material-colors.md).

The builder takes seven badge templates with two variants each from `UNITS.PCX`.
For level `m=0..6`, the source top-left X is `1+24*m`, the two Y values are700
and731, and extraction dimensions are23×30. It copies these fourteen templates
into each owner's bank and replaces decoded template index4 with the owner's
primary color, then index3 with the owner's secondary color. The original
replacement arguments260 and259 are masked to their low bytes by the helper;
they are not additional palette entries beyond255. Other sprite pixels retain
their original values. The existing original extraction/transparency behavior
is not replaced by a two-color rectangle.

The live map caller reads the **Unit owner byte** and selects its owner's bank,
using seven levels and two variants per level. Owner0 selects the first bank.
The ecology-attribution byte is not the owner input to this selection. These
colors specify the map badge/flag recoloring, not native CVR body material,
portrait art, every UI text color or a full faction emblem pipeline.

## Native morale names and live lookup

The installed `alpha.txt` `#MORALE` rows have ordinary text in the first column
and native/Psi text in the second:

| Effective level | Native text |
| ---: | --- |
| 0 | Hatchling |
| 1 | Larval Mass |
| 2 | Pre-Boil |
| 3 | Boil |
| 4 | Mature Boil |
| 5 | Great Boil |
| 6 | Demon Boil |

The live Unit-description helper queries effective morale in mode0 using the
caller-supplied opposing-faction context. For a predefined design below64 with
negative signed weapon power, it chooses the **second** column and resolves
the localized string. Mind Worm design8/weapon12 takes this branch. The test is
the predefined Psi design/weapon predicate, not ecology attribution; a captured
predefined Psi Unit can still use the native column. Ordinary conventional
designs use the first column under this checked branch.

Reuse the [complete native record/lifecycle receipt](ecology-native-unit-records.md):
stored morale1 is not the display level. At turn90, its neutral-context level2
labels Pre-Boil; later level6 labels Demon Boil. The global-turn base schedule
skips5, but label5 still exists and can be reached through other modifiers.
Do not delete it or create a per-Unit age clock.

There is a relevant caller distinction: the map badge path calls the native
level helper with the **current viewing faction as opposing context** for
owner0. The description helper accepts its own context; the traced ordinary
description calls supply0. Existing level controls already show that a nonzero
opponent's retained major atrocities can add1, capped6. Therefore a viewer-
context badge level and neutral-context text need not agree in such a state.
Keep the query context explicit; do not hardcode the stored morale or the
ecology-source faction as that context. No new lifecycle query was needed here.

## Evidence boundary

The frozen supplement binds the pinned executable, relevant linear source
windows, installed PALETTE.PCX/UNITS.PCX/alpha.txt hashes, sample coordinates,
file/decoded indices and existing reserved RGB entries. Its sampling and
mapping verifier performs no emulation. It does not claim a live screenshot,
complete sprite extraction/raster execution, hardware/system palette behavior,
SMAX/PRACX equivalence or a native model material tint. The earlier blink phase
and lifecycle receipts remain unchanged. No production or installed assets
were modified; broad research remains paused.
