# Complete nutrient accounting and opening support scratch

`AccountSmacBaseNutrients` implements original `0x4ff0a0` over owned Base/faction
records, including the separate selected and indexed facility contexts. It consumes
upstream intake and transport values, writes accounting results in place and
returns GROWTH scratch. It does not allocate workers, collect nutrient stock,
change population or execute starvation.

## Field provenance and store order

The selected Base supplies owner, signed-byte population, Golden Age, intake,
storage and all result fields. GROWTH comes from that owner's current field;
Children's Creche comes from the independently indexed facility Base. Each
conditional +2 GROWTH write is retained separately.

Incoming nutrients enter intake first. Consumption is population times the
supplied citizen food rule, plus outgoing nutrients. Surplus is their signed
DWORD difference. Arithmetic/storage use explicit defined wrapping, including
the width-discriminating custom-rule controls; there is no C++ signed overflow.

Stock only receives sentinel normalization: negative storage becomes 0 when
surplus is nonnegative; zero storage becomes -1 for negative surplus. The shared
`NormalizeSmacNutrientStorage` helper is also used by the existing live resource
calculation, preserving its checked intake arithmetic and wide stored-value API.
Neither path adds surplus to storage at this accounting boundary.

Only summary mode exactly 1 adds clamped surplus 0..99 to the selected owner's
aggregate, with original DWORD wrapping. Even an addition of zero emits its
native store. Modes 0 and 2 preserve the aggregate. Incoming intake and mode-1
accumulation make this a mutation pass, not an idempotent inspector query.

## Opening support admission

`PrepareSmacOpeningSupport` implements complete `0x4feb20` for the opening profile
with no Unit belonging to the selected Base owner. It scans owner byte 14 before
interpreting anything else, so poisoned foreign/native Unit fields do not acquire
extra validation requirements or support charges. It clears the twelve incoming,
outgoing, support and convoy scratch words in original instruction order.

Own Units require the general support/convoy traversal and explicitly reject in
this opening API. It neither guesses a free-unit allowance nor treats an overall
nonempty Unit pool as evidence that opening support is nonzero. Foreign Units can
already exist when a later faction founds its first Base.

## Original evidence and validation

Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
All files in both frozen manifests were verified before numeric fixture import:

- `independent-startup-nutrient-accounting-evidence-20261020`, 207 files,
  manifest `7a9aa56533779e417217a999a301761f0aa81b8d96e4ba379553bc9be57e6e72`:
  192 complete three-Base/eight-faction controls, GROWTH scratch and 986 ordered
  stores. They distinguish context, mode, signed-byte population, stock sentinels
  and wrapping arithmetic; extreme width controls do not imply legal campaign
  populations or custom-rule admission.
- `independent-startup-opening-support-evidence-20261020`, 212 files,
  manifest `660a071ca34a3d6ed96f9ee209bb3d6d0b77513eae45a44dc502b47aea6ec3f0`:
  30 complete two-Base/eight-faction/foreign-Unit controls and 360 ordered scratch
  stores. All unowned records survive unchanged, including irrelevant poisoned
  fields and a deliberately different indexed facility context.

No native helper outputs are supplied as inputs. Purchased assets and full native
capture buffers remain outside Git. Both passes accept no RNG stream. Observers
are synchronous; failing mutation passes must remain unpublished staged state.

Cached incremental build and 14 selected native groups pass, including existing
live canonical resource-intake callbacks, source restoration and resource display.
Both changed production translation units pass correctness analysis. A focused
four-TU ASan/UBSan build passes the same groups, with other objects uninstrumented
and leak detection disabled. No platform matrix was repeated.

Worker/specialist intake, owned support/convoys, mineral/energy/citizen accounting
and complete resource orchestration still need composition before these records
can close the generated Base milestone. The complete seven-faction and live
save/network gates remain in the
[integration plan](native-generated-world-integration.md).
