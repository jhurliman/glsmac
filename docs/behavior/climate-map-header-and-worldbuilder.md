# Map climate-extra and world-builder input domains

When starting from an installed map, retain **climate-extra from header offset
`0x10`**, immediately after applied sea offset at `0x0c`. These are separate
signed 32-bit values. Standard Planet stores zero for both. The temperature
category remains at header offset `0xa40`, as documented in the
[initialization handoff](planet-ecology-v5-header-fields.md).

Blank-map initialization resets climate-extra and applied sea offset to zero.
Map loading instead reads the saved header, allocates/recomputes geometry, then
reads tiles and coarse-map bytes. The map wrapper performs its presence/visibility
cleanup without applying the blank-map climate reset. Preserve that distinction:
do not run the blank-map reset over a loaded header before climate preparation.
The source-mapped standard-Planet menu path loads, recalculates climate, then
enters faction setup; it does not insert a blank-map reset between those calls.

A new complete original map-load control starts with climate-extra 777 and
changes only the copied map's header word to −7 in memory. The loader returns
success with climate-extra −7, applied offset zero, and no random calls. Installed
files remain untouched. This proves the loader retains a nonzero signed value;
it is not a new complete campaign or climate-output control.

World-builder values are **loader-clamped**, rather than arbitrary signed words
retained directly. Row numbers below count numeric entries from one:

| Row | Meaning | Inclusive loader domain | Installed value |
| --- | --- | --- | --- |
| 11 | Latitude divisor | 1–64 | 14 |
| 12 | Temperature-band divisor | 1–64 | 14 |
| 13 | Jitter divisor | 1–64 | 8 |
| 14 | Warming divisor | 1–64 | 8 |
| 16 | Mountain shadow | 0–20 | 5 |
| 17 | Hill shadow | 0–20 | 3 |
| 18 | Shadow step | 0–8 | 1 |

The original rules loader compares parsed integers with signed comparisons and
saturates to these endpoints before storing them. The climate consumers read the
stored values; the temperature routine performs signed division by the four
stored divisor values. Apply this normalization at the rules-loading boundary.
The previously frozen semantic model consumes already normalized rules.

Three bounded original loader-block controls cover installed inputs, below-lower
inputs including the signed minimum, and above-upper inputs including the signed
maximum. Parsed numeric token returns are supplied; the original comparisons
and stores execute. This does not add a complete text-parser or nondefault climate
map matrix. [Literal cases](climate-map-header-and-worldbuilder-cases.json).
