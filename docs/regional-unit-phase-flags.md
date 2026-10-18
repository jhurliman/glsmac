# Retained regional unit-phase flags

The actual post-Base owner summary now retains the unit phase's contribution
bits alongside its regional unit and attack counters. This field deliberately
does not represent the final regional flag word: Base-stage, foreign-faction
and strategy writers still need their own integration.

The [original unit-scan controls](behavior/regional-conventional-unit-scan.md)
establish these writers across all 128 regions:

- Plan8 sets bit2 for a Colony Pod; another Pod adds bit2048.
- Plan11 sets bit4096 for Probe presence.
- Plan3 outside a valid Base sets bit8192. Base validity is independent of its
  owner's equality with the Unit owner.

The live Scout/Rover/Former/Colony caller uses its bound design and actual Base
registry. Probe and additional diagnostic designs are covered by the pure
producer controls; their gameplay admission is not implemented by this change.
The summary replaces only the incoming owner's row, after successful preparation,
and preserves every peer's retained flags.

Simulation V19 stores an optional flag row for each positive faction slot.
Fresh worlds initialize these rows explicitly. V18 restore preserves absent
flag history while retaining its existing unit/attack counters; a real owner
summary can subsequently establish that owner's row. Earlier simulation versions
continue to preserve absent military history. Restore and read-only queries do
not manufacture summaries. The codec rejects unknown contribution bits, malformed
presence, truncation and trailing data.

The same 130 literal native projections now compare all 49,920 unit, attack and
unit-phase flag values. The production accumulator passes those projections
under AddressSanitizer and UndefinedBehaviorSanitizer. Live tests cover two Pods,
an outside-Base Scout, peer history, failed preparation rollback, real Unit
removal, stale save/Start restoration and the following owner recount. Focused
codec/migration and affected startup/upkeep/capture/restart tests cover the saved
lifetime. Targeted C++ correctness analysis covers the five affected production
translation units. Full regional strategy and automatic Blind Research remain
unfinished.
