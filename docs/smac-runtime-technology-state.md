# Owned SMAC runtime technology state

`faction::SmacTechnologyState` owns the first mutable faction technology values
needed by the real startup/founding path. It is separate from frozen technology
and faction profiles. This slice adds a producer, a bounded codec and an opt-in
production caller; it does not yet install the value in `Game` or publish it in
a world download. The next live admission boundary is specified in
[the simulation roster plan](simulation-roster-binding-plan.md).

## Admitted state and initialization

Every record has an explicitly supplied simulation ID 1..7 and a canonical
frozen faction source ID. Source order is preserved; neither the template index,
lobby slot, GSID nor display name determines the simulation ID. The input may
admit one through seven records, including different simulation IDs using the
same source profile. Missing, duplicate or out-of-range simulation IDs and
unknown source profiles reject. The producer never automatically admits seven
factions. Planet/native simulation ID0 is outside this first value.

Each record owns 89 possession flags, signed32 technology score, Future Tech
counter and accumulated labs, an optional catalog target 0..88 and an optional
signed32 cached threshold. Absent target/cache represents the original -1
sentinel; a present threshold -1 is rejected to keep one encoding. Other signed
counter/threshold values survive unchanged. This is structural persistence,
not an eligibility policy or a promise that every such state is reachable.

`NewWorld` validates the explicit admission list, constructs a fresh result,
clears possession once across that entire admitted roster, then invokes
`InitializeFactionAtTurnZero` for each recipient. The latter resets score,
Future count and labs to zero, clears target/cache, and applies the source
profile's ordered `NamedTechnologyReferences()` projection. That projection
already has the shared eight-record budget. It preserves existing possession
and all other recipients; it never performs a second global ownership clear.
Both stages are explicit new-world operations, not general research APIs or
live operations guarded by a game turn. The later Game integration must invoke
them only at its admitted turn-zero checkpoint.

RE-SMAC-START-TECH-001 establishes the implemented behavior:

- Named grants admit all 89 catalog IDs, including disabled definitions and
  Future Tech, without parent-ownership checks.
- Duplicate grants remain idempotent; another faction's ownership is retained.
- Named None/Disable references grant nothing. Numeric extra-TECH metadata is
  not a named ID and is not awarded by this producer.
- Turn-zero grants leave score, Future count and labs zero and the target/cache
  absent. No ordinary score, Secrets, later research selection or availability
  query runs here.

The original reference clears every faction marker, including its native slot.
The production value clears only the explicitly admitted playable records it
owns. No native-slot runtime behavior is claimed. This is neither the isolated
nonzero-turn reset block nor the later first-command controller composition.

## V1 codec and restoration

The payload is framed with integer magic `0x534d5453`, exact version1 and a
record count1..7. Each record stores simulation ID, source faction ID, 89 framed
booleans, the three signed32 counters, then presence/value frames for target
and cached threshold. The full payload is limited to8KiB before Buffer
construction; count validation precedes reserving records. Typed frames,
canonical booleans, integer widths, all IDs/links, duplicate identities and
trailing bytes are checked. Old/unversioned and unknown versions reject.

Decode builds an owned temporary and validates it completely before returning.
A caller can replace its existing value only after successful decode; malformed
late fields leave that destination unchanged. Encode likewise validates the
complete value. Input bytes and supplied profiles may be destroyed after decode;
no borrowed links escape. Copying or moving a state preserves its values without
sharing mutable possession storage.

Restoration **never calls the new-world producer**. It preserves saved ownership
and progress exactly, including acquired technologies absent from startup
bonuses, absent/present optional fields and signed endpoints. The codec validates
catalog/source links but does not embed or fingerprint the frozen definitions.
The future world-body integration must use the exact profiles already accepted
by the world header; passing an unrelated catalog is not a compatibility promise.
This is not a disk-save format or host/client lockstep claim.

## Original comparison and validation

The optional `tools/tests/smac_technology_state_probe.cpp` is a caller of the
actual producer, initializer and codec. It parses an explicitly supplied SMAC
alpha source and supplied bonus pairs through existing production parsers. The
comparison script imports no original oracle or private probe code. It schedules
inputs from the fingerprinted public fixture and compares only literal `actual`
outputs in the retained original report.

| Input | SHA-256 |
| --- | --- |
| Original executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Installed alpha | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
|135-case input fixture | `f1b22047c99a1f0d672e2cd029b8bd57800ac764c9117b97b5e3541775a7208e` |
|128-result original report | `c086ce624d47077f2985128c451b20f7f6cb701fc670437f12bb5471e8d76669` |

The report's128 executed cases are an ordered subset of that same135-case
fixture. The supported comparison excludes21 isolated turn-guard cases and
seven controller cases, producing **107/107 matching reset/grant projections**
and107 exact production codec roundtrips. Compared outputs are playable ownership,
final research fields and successful global clear. Ordinary availability,
private provenance/cleanup, native faction0, intermediate reset instruction
ordering and the later controller are not reproduced by this comparison.

Build and run the Release probe from repository root:

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Isrc src/common/Common.cpp src/types/Buffer.cpp \
  src/game/backend/faction/SmacTechnologyState.cpp \
  src/game/backend/settings/SmacTechnologyRules.cpp \
  src/game/backend/settings/SmacFactionStartupRules.cpp \
  src/loader/txt/SmacRulesTXTLoader.cpp src/loader/txt/FactionTXTLoader.cpp \
  src/loader/txt/TXTLoader.cpp \
  src/util/String.cpp tools/tests/smac_technology_state_probe.cpp \
  -o /tmp/smac-technology-state-probe
python3 tools/reference/compare_smac_technology_state.py \
  --probe /tmp/smac-technology-state-probe --alpha "$ALPHA" \
  --report "$ORIGINAL_REPORT" --fixtures "$PINNED_CASES" \
  --output /tmp/technology-state-comparison.json
```

The stream contains recipient, optional additional owner (0 means absent),
optional seeded other owner, five dirty research fields, a bounded bonus count,
then quoted keyword/value pairs. It explicitly admits all seven playable IDs
for this evidence projection; that fixture admission is not runtime policy.

Source validation on the dedicated branch:

- Six native groups pass normally with allocation tracking and with
  ASan/UBSan/LSan plus allocation tracking. They cover explicit admission,
  separate clear/reset stages, all89 IDs, the shared record budget, exact owned
  restore, late malformed records, every truncated prefix and signed endpoints.
- Full normal CTest8/8 passed in62.84s; full ASan/UBSan/LSan CTest8/8 passed
  in116.05s with leak detection enabled.
- Explicit correctness analysis of the one new production TU passed with zero
  failures in2.11s. Existing production TUs are unchanged beyond registration.
- Copied-production controls clear only the first recipient, clear possession
  during recipient initialization, or replay initialization after restore. Each
  fails its intended native assertion. The checkout and ordinary binaries were
  never replaced by a mutant.

Receipts are retained outside Git under
`/home/jhurliman/src/glsmac-unit-weapon-parser/technology-state-*`. Original
files, reports and proprietary data are not included in the repository.
