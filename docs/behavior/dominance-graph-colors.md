# Dominance graph colors: initialized native slots and canonical binding

**The graph tables are initialized global slot tables, not per-faction art
tints.** Under the admitted default seven-row `FACTIONS` startup, an explicit
canonical-profile→native-slot mapping is valid. Keep the complete seven-slot
history roster even when current accepted participants use dense simulation IDs.

The new controls execute the native table producer over poisoned entries,
then two complete native faction-list loads: original default order and a
Gaia/University row1/3 swap in the emulated file stream. Fourteen complete
faction text parsers and actual matching PCX loads run. Neither load writes
the four slot-color tables or active palette. **21 native graph color-selector
fragments** reproduce default, swapped and sparse-active-slot colors. Graph
geometry and score/history writers were not rerun.

## Producer and initialized values

The global asset initializer loads external `PALETTE.PCX` with index offset10
and palette-install argument0. For slot`i=0..7`, sample loaded pixel indices
at`(580+8*i,23)` for normal colors and`(580+8*i,31)` for alternate colors.
Store the results in the corresponding slot. These are indices, not sampled
RGB values or faction-file body material colors.

| Native slot | Default canonical definition key | Normal index | Alternate index | Default graph index |
| --- | --- | --- | --- | --- |
| 0 | No plotted faction series | 249 | 1 | unused |
| 1 | `GAIANS` | 241 | 32 | 241 |
| 2 | `HIVE` | 252 | 187 | 252 |
| 3 | `UNIV` | 255 | 209 | 255 |
| 4 | `MORGAN` | 251 | 85 | 251 |
| 5 | `SPARTANS` | 65 | 79 | **79** |
| 6 | `BELIEVE` | 102 | 109 | 102 |
| 7 | `PEACE` | 235 | 186 | 235 |

The graph uses normal`[slot]`, except when the retained graph key at the
faction text record's`+0x1c` equals case-sensitive **`SPARTANS`**; then it uses
alternate`[slot]`. This is a retained key comparison, not translated name,
leader name, human status or an assigned simulation ID. In the checked stock
definitions, this key matches the PCX basename field at record`+4`; the fields
should not be conflated for arbitrary custom definitions.

The executable's pre-initialization values differ. In particular, its normal
slots1/7 initially contain111/187, and alternate slot5 contains57. Those are
the explicitly declared inputs of the earlier graph-geometry fixtures.
After this native producer, canonical gameplay uses241/235 and **79** instead.
Do not promote the old pre-producer defaults into production constants. The
old geometry/writer evidence is preserved unchanged; the graph reference takes
color tables as inputs, so its coordinate proof remains applicable.

## Lifetime and swap discriminator

The native producer performs32 stores across four eight-entry slot tables;
the first two are the graph's normal/alternate tables. Re-running global
asset initialization derives them again from the palette metadata. They remain
unchanged through the checked default and custom list/text/faction-image loads.
Faction PCX color metadata used by actor materials or the territory CLUT is
a separate path and does not assign these graph slot colors.

The source audit finds the producer's two stores as the only direct stores to
the normal/alternate tables in the pinned executable. Actual memory watches
also cover all four tables throughout both list/parser/PCX paths. Actor/cache
material generation and sprite storage retain the same declared presentation
boundaries as the earlier minimap discriminator; this is not a new full-cache
wrapper execution claim or a general custom-palette lifetime survey.

The swap is decisive:

| Loaded slot | Default contents / graph index | Swapped contents / graph index |
| --- | --- | --- |
| 1 | `GAIANS` /241 | `UNIV` /241 |
| 3 | `UNIV` /255 | `GAIANS` /255 |
| 5 | `SPARTANS` /79 | `SPARTANS` /79 |

Thus a University profile loaded in native slot1 is green-index241, while
default-order University in native slot3 uses255. Its art filename and display
name cannot recover a lost source slot in arbitrary reordered games.

## Port binding for the admitted canonical startup

The complete original default list load establishes the seven definition keys
and ordinal slots in the table above. The existing
[setup-choice transfer](setup-wizard-choice-transfer.md) separately proves that
selecting a human faction preserves its loaded slot into initialization rather
than renumbering it as human1. Therefore the frozen canonical preset can carry
an explicit source-slot mapping keyed by its immutable canonical definition
identity. This is source order, not inference from user-facing names.

For example, if default-order `UNIV` becomes simulation1 and `GAIANS` becomes
simulation2, the mapping is`{simulation1: native3, simulation2: native1}`.
Use native3/255 for the former and native1/241 for the latter. Do not index
these tables directly with dense simulation IDs.

Build the graph's full identity/color roster from all seven frozen source
slots, separately from accepted/current participants. Plot the seven retained
history columns by native slot, preserving old samples for currently inactive
or absent participants. The graph's existing loop has no active filter; the
new sparse-active-mask control also selects colors for all seven slots without
changing them. A fresh history's never-used columns remain its initialized
zeros; restored historical columns must not be dropped or reconstructed from
today's participant list.

This canonical adapter is valid only when the frozen startup explicitly admits
the default `FACTIONS` order. Custom order or native-save import needs explicit
loaded-slot provenance or an equivalent saved assignment. A profile's identity
alone cannot distinguish the default University3 case from swapped University1.
The portable helper rejects canonical inference when that order is not admitted.

[Cases](dominance-graph-colors-cases.json) and the
[reference](dominance_graph_color_reference.py) include the produced tables,
default/swap discriminator and canonical/sparse mapping checks. Run the
reference without assets for those checks; add`--palette-pcx` for independent
external-PCX metadata reconstruction. No original palette/image pixels are
embedded. Prior graph geometry, history writer and minimap evidence remain
unchanged.
