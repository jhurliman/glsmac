# Frozen ecology inputs and Planet metadata

The session now reads the original `#WORLDBUILDER` Sea Level Rises value from
numeric row15, clamps the signed32 source value to1..100, and freezes that
loaded value. This follows the [native loader receipt](planet-ecology-global-inputs.md).
Installed `alpha.txt` yields5; synthetic source19 remains19 through host transfer.
Missing legacy source authority stays unavailable. Malformed numeric fields,
duplicate sections and truncated wire profiles are rejected.

Global settings V20 appends the optional profile. V17–19 decode with no ecology
profile; they do not read today's installed rules to invent missing history.
Replacement/removal of a frozen profile is rejected before settings publication.
World header V34 binds the same optional authority before its world body. V32/33
remain compatible only with sessions lacking that authority. A saved world with
another loaded Sea Level Rises value cannot be admitted into the current session.

The bounded Planet v5 reader exposes three independent signed little-endian
header words, following the [field-layout receipt](planet-ecology-v5-header-fields.md):

| Getter | Header byte offset | Installed Planet |
| --- | --- | --- |
| `CumulativeSeaLevelOffset()` | `0x00C` | 0 |
| `CloudCoverCategory()` | `0xA44` | 1 |
| `NativeLifeCategory()` | `0xA48` | 1 |

These are raw metadata getters. They preserve signed words and every source
byte; category admission remains distinct from byte decoding. They do not
quantize the port's continuous map-generation settings or claim that arbitrary
negative category values are legal original-game selections.

The next live integration must own mutable sea offset and per-faction clean
mineral/atrocity/ecology aggregate history, respect the measured initialization
and owner-pass boundaries, and connect the existing ecology calculations to
both resource passes. The original [perihelion ordering](perihelion-global-turn-order.md)
is now established: calendar update, maintenance, then owner upkeep. This change
does not yet publish Base eco-damage, generate blooms, or implement climate events.

Validation: cached incremental build;32 focused native groups including installed
Planet and alpha.txt, source clamp/sign/position tests, bounded codec failures,
frozen settings and world identity, legacy absence, and actual campaign file
restore followed by an actual turn. Targeted correctness analysis passes for
the four changed production translation units. Existing successful analysis of
unchanged units is reused; no full platform matrix was repeated.
