# Owned SMAC fungal capability producer

`SmacTechnologyState::FungalCapabilities(simulation_id, technologies, profiles)`
derives an owned nutrient/mineral/energy triple from the existing authoritative
technology possession and frozen faction profile. It does not store a second
mutable copy. Repeated calls, ownership clearing, explicit ownership changes and
exact state restoration all use the current supplied sources.

This is the input producer needed by the existing tile nutrient/mineral/energy
calculators. It does not activate their live base/worker callbacks, choose workers
or specialists, schedule technology awards, or change default gameplay. No
technology-state, faction-profile, Settings, Simulation or world wire version
changes in this slice.

## Sources and calculation

The method validates the complete existing technology state and both profiles,
then selects an explicitly admitted simulation ID1..7. The canonical faction
source comes from that owner's admission. Source order, lobby slot and display
name never stand in for simulation identity. Missing/native ID0 is rejected;
this API does not construct native-faction capabilities.

For each owned catalog row0..87 with a fungal flag, resolve both prerequisite
tokens through the existing catalog resolver. Unknown relevant tokens reject.
A disabled first prerequisite excludes the record. A disabled second excludes
it unless the first is None. Known prerequisite technologies need not themselves
be owned: this is record validity, not research-chain traversal. Row88 does not
contribute, even if owned and flagged. Unowned or unflagged rows do not require
this capability-specific prerequisite query.

Each eligible row contributes one for each active flag: nutrient `0x100`, mineral
`0x80`, energy `0x40`. These are loaded bits, not hardcoded technology-name
exceptions. The producer next visits the faction profile's ordered `Records()`
projection, which already applies the shared eight-record limit. It applies
FUNGNUTRIENT, FUNGMINERALS and FUNGENERGY amounts to the corresponding sums,
then floors each complete sum at zero. There is no upper99 cap in the builder.

Every evaluated addition uses int64 and must fit signed32 before proceeding,
including a negative intermediate that would later be floored or offset by
another modifier. This is explicit port admission for original overflow that
FUNGAL001 did not establish. Unknown reference rejection is likewise a strict
port policy, not emulation of an original diagnostic dialog. Failures cannot
change the const source state or publish a partial output.

PLANET, ECONOMY, facilities, projects and local/global resource events are absent
from this API. The existing tile calculators separately apply effective PLANET,
the0..99 fungal yield clamp and their resource-specific event tails. In
particular, Gaia's owned Ecology plus FUNGNUTRIENT1 produces2/0/0; positive
PLANET does not add a third nutrient here.

All three fungal keywords consume one shared record, including a zero amount.
Repeated records retain their order; later records drop after eight. Numeric
nonzero TECH still assigns its separate scalar after the list fills, while
named TECH requires a record slot. Other unsupported keywords still reject,
including beyond capacity. Strict numeric-prefix/overflow and whole-file
lexical admission remain the existing port policies; the new parser receipt
does not establish additional original malformed-input behavior.

The mineral/energy extension appends faction typed tags 14/15. V2 field order,
shape and tags 0..13 stay unchanged. Older readers reject the new tags; there
is no silent downgrade. Settings and world exact equality include these
fields, even when the eight-record projection would drop them. This is an
appended-variant compatibility boundary, not a claim that an older reader can
load a new profile containing the new variants.

## Original evidence and comparisons

The human-owned contracts are
[FUNGAL001](behavior/fungal-resource-capabilities.md) and
[FIRST-WORKER-CONTEXT001](behavior/first-base-worker-context.md), plus
[FUNGAL-PARSER001](behavior/fungal-bonus-parsing.md).
FIRST-WORKER runs seven separate controlled local difficulty3 worlds, each named
human faction at simulation1. Complete-loader-produced fields are transferred
between isolated original guests; neither it nor this comparison claims an
uninterrupted full-game startup.

`project_fungal_cases.py` checks all three fixture hashes and emits only retained
semantic inputs and literal original outputs:

- FUNGAL fixture SHA256
  `b33de468151434c5de12af575ae92e3c52b298026cd30e7ab3d0bca3ac211a4e`.
  Twenty-four of 25 named cases are admitted;014 is native faction0. The
  private 384-case grid has no retained literal output rows in this comparison
  and is not claimed as covered or rerun.
- FIRST-WORKER fixture SHA256
  `84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab`;
  bound receipt SHA256
  `751139dbb2cedb5774ba38e08b0637fdd2ee403443e8263d16ee7bf1a874d910`.
  All seven observed possession sets produce their captured capabilities.
  University's later choice0 is supplied possession, not a newly invented
  chooser or an extra grant in `NewWorld`.
- FUNGAL-PARSER fixture SHA256
  `7387c3656e1decedabf8a14c7e72d19f27289df404595d62bf7e46b6751b023a`;
  original report SHA256
  `af208b21351778834f2a46ebeb44efdcb2fa1e7bcd72519843d2b2908d1edf9a`.
  All twelve supplied token sequences pass through the actual production
  faction-file parser, record projection, capability producer and tile
  calculators. Expected ordered records and outputs come from the original
  parser-to-builder capture. Original unused record storage is normalized to
  the port's canonical zero field; original kinds map to the explicit port
  tags. Possession is explicitly empty, so parsed Ecology is not granted.

The 24 native cases compare both the produced triple and its composition with
the real existing nutrient/mineral/energy calculators on the explicitly
supplied qualifying fungal tile. Their neutral terrain/facility context comes
from those semantic fixtures; it is not a live missing-state default. Separate
tests cover owner isolation, clear/grant changes, exact restoration, detached
result lifetime, the eighth/ninth record boundary, final-only flooring,105 and
signed32 limits, and invalid/unsupported inputs.
The added parser cases distinguish mixed-resource budget sharing, repetitions,
signed/zero amounts, interleaved SOCIAL/named TECH, and numeric TECH after
capacity. New tags roundtrip through faction and settings codecs, and changed
late tags fail world validation before body reads. The existing real Client
final-download rejection matrix also exercises both tags in immediate and
queued SMAC/SMAX paths before Map allocation.

The opt-in Release/UBSan probe parses the actual installed alpha and seven
faction files through production parsers, supplies the seven captured complete
possession sets, calls the real producer, and restores catalog/profile/state
before recomputing. The comparator checks installed source hashes against the
bound receipt. All7 outputs match. Five malformed probe requests reject.
No original executable is rerun by either tool, and proprietary files remain
outside Git.

```sh
python3 tools/reference/project_fungal_cases.py \
  --fungal /local/research/fungal-resource-capabilities-cases.json \
  --worker /local/research/first-base-worker-context-cases.json \
  --parser /local/research/fungal-bonus-parser-cases.json \
  --output /tmp/SmacFungalCapabilities.h
python3 tools/reference/compare_smac_fungal_capabilities.py \
  --probe /local/build/smac-fungal-probe \
  --fixture /local/research/first-base-worker-context-cases.json \
  --bound /local/research/first-base-worker-context-bound-20261017.json \
  --installed /local/AlphaCentauri --output /tmp/fungal-comparison.json
```

The standalone probe links the two source parsers, technology/faction profile
and owned technology-state implementations plus their Buffer/TXT/String/Common
dependencies. Compile every linked dependency consistently with Release/NDEBUG,
UBSan, function/data sections and linker garbage collection; the retained local
build driver records the exact compile commands. Mixing Debug/Release Class or
Buffer layouts is not a supported probe configuration.

## Validation

The initial nutrient-only producer at `3dced2ba` was validated on Linux
Clang15. Twenty focused native groups passed
with allocation tracking in normal and ASan/UBSan/LSan builds, including all
five new fungal groups and the existing faction/technology-state groups.
Five copied-production controls fail their intended assertions: counting
unowned flags, including row88, losing the None exception, bypassing the shared
record budget, and capping the builder at99. The tracked production source is
never mutated for these controls.

Its full normal CTest passed8/8 in87.01s; full ASan/UBSan/LSan CTest passed8/8 in
182.21s with leak detection and halt-on-error enabled. Full LLVM15 analysis of
all338 production translation units reports zero failures in136.82s; the
Windows-only network implementation remains outside that Linux analysis.
Fixture regeneration is byte-identical, Python compilation and diff checks pass.
Receipts and their hashes are retained in
`/home/jhurliman/src/glsmac-startup-main-refresh/fungal-final-manifest.json`.

That initial compilation found three test diagnostic
expressions incompatible with GT_ASSERT's literal-prefix macro; only those
test messages changed, and its failed log is retained. No Windows, macOS,
hosted CI, original-execution rerun or live worker-consumer pass is implied.

The separate mineral/energy extension passes 28 focused groups with tracking
in normal and ASan/UBSan/LSan builds: 23 faction/state/fungal groups, four real
Client download paths, and one malformed faction-header group. Full normal
CTest passes 8/8 in 87.09s; full sanitizer CTest passes 8/8 in 187.49s, with
leak detection and halt-on-error enabled. Full LLVM15 analysis checks all 338
Linux production translation units with zero failures in 169.29s. The rebuilt
all-Release/UBSan installed probe retains 7/7 outputs and five CLI rejections.
Fixture regeneration remains byte-identical; Python compilation and diff
checks pass.

Four copied-production controls fail their intended assertions: omitting
mineral accumulation, omitting energy accumulation, losing the new tags'
shared-slot consumption, and restoring the old decoder tag ceiling. The last
control first exposed an uncaught test decode exception; its abort log is
retained. The test now converts that exception into an explicit assertion,
and all four controls exit 1 as intended. This test-only diagnostic change
followed the full suites; both rebuilt focused suites pass afterward.
Production was unchanged throughout. The extension receipts and hashes are in
`/home/jhurliman/src/glsmac-startup-main-refresh/fungal-tags-final-manifest.json`.
These are local Linux checks, not new hosted/platform or live-consumer claims.
