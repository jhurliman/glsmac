# Native generated temperature, river sources and initial fungus

`GenerateInitialClimate` continues the owned pretemperature plane through
original `0x5e02be`, immediately before landmark placement. It runs temperature
(`0x5dc060`), river-source placement (`0x5db4f0`), and initial fungus (`0x5db310`),
then marks `Stage::Prelandmarks`. It requires the pretemperature continuation
and generation flags 1/1. Individual kernels support independent raw-plane
controls and leave stage bookkeeping to their caller.

Temperature reuses the existing `smac_climate::Temperature` rules through an
owned view. Row completion publishes only byte 0's low three climate bits,
preserving skipped landmark cells and every other raw field. Optional stream
observers and row callbacks leave existing climate consumers' default behavior
unchanged. The view uses the existing bounded climate geometry admission.

River sources are a separate operation from river routing. The pass clears the
source bit on every tile, computes the nested-truncation quota, samples CRT X
then Y, rejects nearby sources in the ordered 81-cell footprint, and chooses
the first highest band in the ordered 49-cell footprint. It stops after the
quota or 4,000 attempts. Zero quotas still clear every source bit.

Initial fungus constructs native noise with 324 CRT draws and uses sampling
scales 16/256. Life controls the three inclusive admission windows. Mount
exemptions precede polar admission draws; land rockiness rejects after
admission. Two ordered DWORD stores set fungus and then clear the supplied
mask, preserving all other fields. Existing fungus outside admitted cells is
retained.

Settings are explicit resolved categories `[4]` temperature, `[5]` rainfall,
`[6]` native life and `[1]` ocean; extra climate is `reset_words[1]` and the
caller supplies the fungus clear mask (ordinary `0x288050`). The same world
rules and RNG continuation flow from the previous generation stage.

## Original comparisons

All 22 original controls match complete 44-byte tile planes, outgoing RNG,
**15,029 ordered stores**, **10,584 ordered CRT draws**, **5,469 ordered shared
seed/draw events**, and row checkpoint counts. Controls cover climate settings,
Crater and Mount detail/sign edges, feature warming, source quotas and the
4,000-attempt cap, flat maps, nested quota truncation, native life and rockiness.
Numeric input recipes are checked against original input hashes.

The actual original setup test generates from zeroed storage and seed 1 through
the complete pretemperature pipeline, then checks each return boundary:

| Return | Complete plane SHA256 | CRT | Shared |
| --- | --- | ---: | ---: |
| Temperature | `61e207f67cc7e36738d51bacce5128800819a1c76d01d96878e6ad605d815b63` | 3828047505 | 193599486 |
| River sources | `56a34d78d30cf7f4bafb616d647cfa584ce85331f91dc86a6816b66974e9a57d` | 3911652965 | 193599486 |
| Initial fungus | `cb9258c37a687924a6d8032b5d96ac4b63d7d195835b90985b72a86358fd48cb` | 2128787453 | 193599486 |

These compare all **140,800 bytes**, **8,514 ordered stores**, **572 CRT draws**
and **6,401 shared seed/draw events** across the three passes. Store and RNG
traces are compared separately, without original program-counter fields; this
does not assert one globally interleaved instruction trace. The composed
controller independently reaches the same plane and RNG boundary.

Validation: cached incremental build; twelve affected native groups, including
existing climate regressions; correctness analysis of the new TU, shared
climate TU and three known production consumers; focused eleven-TU ASan/UBSan
run. All pass. No local platform matrix repeated.

Evidence: executable SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`;
44-file frozen `native-initial-climate-generation-evidence-20261020` manifest
SHA256 `26f979da88895dd9f772079601bdba57273e75aad833283b9da52d5712550781`.
Every file was verified before fixture extraction. Purchased assets stay
outside Git; no captured intermediate outputs are production inputs.

The new source handoff also corrects an earlier editor-branch description:
`5dbe60`'s editor-only call to `5a8f20` writes byte 0's climate bits. Rockiness
uses `5a9010` and byte 5. Ordinary generation flags and accepted results are
unchanged by this naming correction.

## Remaining work

Landmark placement, later climate/routing and final preparation, and faction
startup remain before this intermediate state becomes a completed generated
world. The live procedural path and rendered gameplay remain unchanged.
