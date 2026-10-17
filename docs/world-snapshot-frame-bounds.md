# World snapshot frozen-frame bounds

The world header now bounds every frozen-profile outer string before
`Buffer::ReadString` allocates or advances past that frame. The format and
version are unchanged.

| Field | Maximum payload bytes | Existing constraint |
| --- | ---: | --- |
| Ruleset ID | 4 | `smac` or `smax` |
| Production, technology, world-size catalog, faction startup | `Network::MAX_PACKET_BYTES` | Each belongs to the already bounded complete global-settings packet |
| Accepted world-size selection | 56 | Four 14-byte integer frames: version, category, width, height |
| Predefined units | 24 KiB | Existing profile codec budget, unchanged |
| Movement rules | 28 | Existing two-integer codec budget, unchanged |

The network ceiling is `65536 - sizeof(int)` (65,532 on the supported
platforms), including neither a second guessed constant nor a new gameplay
restriction. `InitializeHostRules` stages and serializes the entire settings
payload before freezing it; State admission additionally includes faction
metadata and the actual Packet envelope. Thus no profile from an accepted
host can exceed the new conservative outer ceiling.

The writer encodes and validates all profiles and their frame sizes before
appending its first byte. Nested codecs still check versions, checksums,
counts, values and exact frozen equality. A complete exact-cap malformed
payload passes the outer size gate and fails its checksum or codec; it is not
misreported as oversized.

This limits nested header materialization, not the already assembled download
buffer or the map/body payload. It does not promise rollback after a malformed
world body, change allocation policies of unrelated codecs, or add disk-save
compatibility.

## Regression boundary

Complete cap+1 and twice-cap frames, with both correct and damaged outer
checksums, reject with the length-limit diagnostic and leave the cursor at the
frame start. Separate unbounded test reads establish that correct-checksum
frames and their following body were complete. Exact-cap cases distinguish
outer admission from subsequent rejection; existing valid SMAC/SMAX headers
still preserve all body bytes.

The actual Client final-chunk handler runs both immediately and through its
queued owner callback. The fixture seeds previously received full chunks and
delivers a final packet within the real transport cap. It preserves the prior
map pointer/bytes, settings, managers, RNG, turn/completion flags and callback
counts on rejection. This is the real packet dispatcher and decoder with a
seeded download prefix, not socket or earlier-chunk I/O coverage.

Actual oversized source profiles are rejected by host admission before freeze.
To exercise the writer's additional defensive guard, the test then deliberately
alters a valid, nonconst test-owned frozen value through its const getter. This
synthetic corruption is not a supported settings mutation or a claim that the
host can publish such a profile. Writer rejection leaves existing output bytes
and cursor unchanged.

## Validation

Source parent: `f0b1ee2e7802b984e60c5de1116ae74afea4a8dd`.
Normal Linux CTest passed 8/8 in 79.19 seconds before the final test-only
transport-sized final-chunk strengthening. The final seven focused native
groups pass with allocation tracking. LLVM 15 analysis of the one changed
production translation unit reports zero failures (2.10 seconds).

Two copied production translation units independently remove the reader caps
or writer preflight. Each fails its intended early-boundary assertion with
exit1; intended checkout sources are never replaced. Receipts are retained
outside Git under `/home/jhurliman/src/glsmac-unit-weapon-parser/world-frame-*`.
No new sanitizer, native Windows/macOS or rendered-game result is claimed by
this narrow follow-up.
