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
projection, which already applies the shared eight-record limit. This first
slice applies its supported FUNGNUTRIENT amounts, then floors each complete sum
at zero. There is no upper99 cap in the builder.

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

FUNGMINERALS and FUNGENERGY remain rejected by the unchanged faction parser,
even after its shared record capacity. The newly captured parser extension is
a separate follow-up; this commit does not silently add those keyword/wire
variants. Technology flags can already produce all three resources for the
canonical seven admitted profiles.

## Original evidence and comparisons

The human-owned contracts are
[FUNGAL001](behavior/fungal-resource-capabilities.md) and
[FIRST-WORKER-CONTEXT001](behavior/first-base-worker-context.md).
The latter runs seven separate controlled local difficulty3 worlds, each named
human faction at simulation1. Complete-loader-produced fields are transferred
between isolated original guests; neither it nor this comparison claims an
uninterrupted full-game startup.

`project_fungal_cases.py` checks both fixture hashes and emits only retained
semantic inputs and literal original outputs:

- FUNGAL fixture SHA256
  `b33de468151434c5de12af575ae92e3c52b298026cd30e7ab3d0bca3ac211a4e`.
  Twenty of25 named cases are admitted. Cases009/011/023/024 require excluded
  mineral/energy faction modifiers;014 is native faction0. The private384-case
  grid uses those excluded modifiers and is not claimed as covered.
- FIRST-WORKER fixture SHA256
  `84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab`;
  bound receipt SHA256
  `751139dbb2cedb5774ba38e08b0637fdd2ee403443e8263d16ee7bf1a874d910`.
  All seven observed possession sets produce their captured capabilities.
  University's later choice0 is supplied possession, not a newly invented
  chooser or an extra grant in `NewWorld`.

The20 native cases compare both the produced triple and its composition with
the real existing nutrient/mineral/energy calculators on the explicitly
supplied qualifying fungal tile. Their neutral terrain/facility context comes
from those semantic fixtures; it is not a live missing-state default. Separate
tests cover owner isolation, clear/grant changes, exact restoration, detached
result lifetime, the eighth/ninth record boundary, final-only flooring,105 and
signed32 limits, and invalid/unsupported inputs.

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

Source validation runs on Linux Clang15. Twenty focused native groups pass
with allocation tracking in normal and ASan/UBSan/LSan builds, including all
five new fungal groups and the existing faction/technology-state groups.
Five copied-production controls fail their intended assertions: counting
unowned flags, including row88, losing the None exception, bypassing the shared
record budget, and capping the builder at99. The tracked production source is
never mutated for these controls.

Full normal CTest passes8/8 in87.01s; full ASan/UBSan/LSan CTest passes8/8 in
182.21s with leak detection and halt-on-error enabled. Full LLVM15 analysis of
all338 production translation units reports zero failures in136.82s; the
Windows-only network implementation remains outside that Linux analysis.
Fixture regeneration is byte-identical, Python compilation and diff checks pass.
Receipts and their hashes are retained in
`/home/jhurliman/src/glsmac-startup-main-refresh/fungal-final-manifest.json`.

The initial compilation found three test diagnostic
expressions incompatible with GT_ASSERT's literal-prefix macro; only those
test messages changed, and its failed log is retained. No Windows, macOS,
hosted CI, original-execution rerun or live worker-consumer pass is implied.
