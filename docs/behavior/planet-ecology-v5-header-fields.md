# Planet v5 global-input header fields

2026-10-18. Addendum to the frozen
[Planet ecology input receipt](planet-ecology-global-inputs.md); that evidence
bundle remains unchanged. [Numeric fields](planet-ecology-v5-header-fields-cases.json).

The serialized map header starts at **file byte15** in the admitted v5
`TERRANMAP` file. Offsets below are relative to that header, not relative to
the magic, version word, tile block or whole file.

| Field | Header byte offset | Whole-file byte offset | Encoding | Installed Planet value |
| --- | ---: | ---: | --- | ---: |
| Cumulative sea-level offset | `0x00C` (12) | `0x01B` (27) | signed32, little-endian | 0 |
| Cloud-cover category | `0xA44` (2628) | `0xA53` (2643) | signed32, little-endian | 1 |
| Native-life category | `0xA48` (2632) | `0xA57` (2647) | signed32, little-endian | 1 |

Each field occupies four bytes. Load the complete word; these are not packed
bytes or bitfields. The native loader transfers the header without a field
conversion, so use little-endian `int32` getters. Ordinary category values
are0..2; this does not admit arbitrary negative or out-of-range category values
as legal settings. The cumulative sea offset is signed and must retain its
sign, independently of the ecology consumer's later clamp.

Header length is `0xAA4` bytes. These locations are distinct from the world-size
category at header`0xA30`. They contain global category/sea authorities,
not per-tile moisture, contour, climate bands or lighting. The full original
loader's earlier poisoned-word control establishes that it publishes these
actual serialized values. The offsets identify the storage schema rather than
replacing getters with the installed asset's constant1/1/0 values.

Scope is the admitted successful v5 installed Planet format. Later versions,
other header formats, validation of malformed categories and renderer behavior
remain outside this addendum.
