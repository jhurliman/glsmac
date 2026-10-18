# Installed Planet ecology and cloud inputs

2026-10-18, stock SMAC. **The full installed Planet map load publishes native
life1, cloud cover1 and cumulative sea offset0.** Both categories mean
**Average**. The subsequent tested new-world/fresh-faction prefixes retain
these globals. [Literal controls](planet-ecology-global-inputs-cases.json).

| Authority | Installed Planet value | Producer |
| --- | ---: | --- |
| Global native-life category | 1, Average | Serialized map header read by the full map loader |
| Global cloud-cover category | 1, Average | Serialized map header read by the full map loader |
| Global cumulative sea-level offset | 0 | Serialized map header read by the full map loader |
| Active WORLDBUILDER Sea Level Rises parameter | Installed rules5 | Rules text loader, separate from the map header |

The installed script choice order is cloud0 Sparse/light rainfall,1 Average,
2 Dense/heavy rainfall; native life0 Rare,1 Average,2 Abundant. These labels
identify stored categories; no new interactive category selection is claimed.
The original native-life field supplies ecology's life factor. Cloud cover is
a separate global input used by research logic; it is not a replacement for
native life in the measured ecology formula.

One full original Planet loader call starts with all three map words poisoned
to777. It returns successfully after consuming the complete installed asset
and leaves1/1/0, with no RNG calls. The original loader requests the actual
serialized header; its memory-backed FILE service copies those bytes. The
observed zero sea offset is **loaded data**, not evidence of an unconditional
map-load zeroing rule. Preserve the offset stored in any admitted map asset.

Two controls retain that imported map and execute the actual new-world parent
through its first faction-initializer entry, then resume the same native stack
through the established fresh-faction reset prefix:

| Inputs before new-world entry | Before first faction initializer, turn0 | After fresh-faction reset prefix |
| --- | --- | --- |
| Cloud1, native1, sea0, previous turn17 | 1/1/0 | 1/1/0 |
| Cloud1, native1, diagnostic sea12, previous turn17 | 1/1/12 | 1/1/12 |

No watched global is written during either prefix. The12 control distinguishes
retention from resetting an already-zero value. These are bounded parent/child
prefixes, not complete initializer returns or a continuous replay of map
preparation, setup dialogs and all faction starts. Existing
[map import](standard-planet-map-import.md) and
[setup transfer](setup-wizard-choice-transfer.md) establish the related loader
and setup caller paths; their scope is unchanged.

Use the **loaded** WORLDBUILDER `Sea Level Rises` field, numeric row15
(zero-based index14), after `Global Warming` and before `Cloudmass peaks`.
One complete native rules load overwrites poisoned777 with parsed installed5.
The loader clamps this field to1..100; the
[ecology consumer](eco-damage-scaling.md) also clamps its supplied value to1..100.
Five is an observation of the installed configuration, not a constant in the
ecology rule. This control uses the actual lexer and installed text, without
supplying a numeric token or rule result.

The cumulative sea offset is an independent global authority in the original
climate system's units. It is not the pending sea change, per-tile contour,
altitude band, moisture, cloud texture or rendered lighting. Existing
[lifetime evidence](ecology-input-lifetimes.md) separately establishes blank-map
creation clearing it, save/load restoring it, and admitted sea-step increments.
Do not run the blank-map reset when importing an already serialized Planet map.
No new terrain climate/lighting output, world generation or ecological event
behavior is inferred from these global input controls. Standing research remains
paused.
