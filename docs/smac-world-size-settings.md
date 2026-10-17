# Accepted SMAC world size

SMAC setup now produces an explicit accepted world-size category and coordinate
span. Presets retain source positions 0 through 4; a custom choice retains 99,
including when its dimensions equal a preset. A missing choice stays missing.
Nothing derives category from area, aspect ratio, map contents or the previous
112×56 port default. This is an input producer for later founding/production
rules; it does not activate those rules.

The source is the host's `alpha.txt` `#WORLDSIZE` section, read during the existing
single host-rules preparation. `SmacWorldSizeRules` owns all five names and source
X/Y values in order. The accepted coordinate width is twice source X; coordinate
height is source Y. Names, including `|` annotations, remain metadata rather than
keys. Installed SMAC rows yield:

| Category | Source X/Y | Coordinate width/height |
| --- | --- | --- |
| 0 Tiny | 24 / 48 | 48 / 48 |
| 1 Small | 32 / 64 | 64 / 64 |
| 2 Standard | 40 / 80 | 80 / 80 |
| 3 Large | 44 / 90 | 88 / 90 |
| 4 Huge | 64 / 128 | 128 / 128 |

This interpretation comes from `RE-SMAC-WORLD-SIZE-001`, including its executed
category producer and scalar loader. That handoff distinguishes custom 99 from
preset identity even at equal geometry. It also documents original random and
saved-preference branches; neither is implemented here. The original executable
was not rerun for this implementation, and its full world allocation was not
part of that handoff.

## Setup and lifecycle

The actual SMAC size menu selects source positions; its Custom Size dialog takes
explicit coordinate spans. OK commits both dimensions and category together.
Cancel and rejected values preserve the previous accepted choice. Quick Start
from the menu now waits for this size choice. The host lobby offers the five
preset categories; its existing authoritative `game_settings` event applies and
rolls back category and dimensions together. A previously accepted custom choice
is displayed as custom. Clients cannot select a host category. Raw scripted
dimension writes invalidate acceptance instead of silently retaining a preset.
The SMAC event rejects duplicate planet-size entries and raw dimension changes
(alone or mixed with a planet-size choice), so rollback has one original accepted
selection. Direct scripted map-dimension setters still invalidate acceptance.
Informational lobby choices restore the displayed native
selection instead of appearing to accept a placeholder.

For command-line quickstart, supply either `--quickstart-world-size 0..4` or
`--quickstart-mapsize WIDTHxHEIGHT` with `--ruleset smac`; the latter explicitly
means custom 99. The options are mutually exclusive. Missing size and SMAC map
files/dumps reject, because those files do not establish an accepted category.
The preset option is rejected for SMAX. SMAX's existing size menu, CLI dimensions
and generation semantics are unchanged; it receives no SMAC profile/category.

`StartGame` validates and locks accepted SMAC size before frontend transfer.
Reset/new ruleset selection clears the acceptance and its lifecycle lock.
Repeated lobby settings may change an unlocked accepted selection. A running
session rejects even a category-only change at identical dimensions. Backend
map acquisition and generation also validate before map/tile allocation. Native
dimension mutation inconsistent with an accepted selection is detected rather
than repaired or reclassified.

## Explicit port limits

This parser's ingestion policy requires a unique section, exactly five complete
three-column rows and a count of five. Numeric fields consume the complete signed
decimal spelling, with checked int32 conversion. Source X is bounded to 2..256
and source Y to even values in 4..256. Odd Y is rejected when parsing or
restoring the catalog: current `Tiles` storage and pole-row processing require
even heights. This guarantees every category exposed by the setup menus and
lobby settings event can be selected before an event is broadcast. Rejected
catalogs publish neither partial host profiles nor peer settings.
Accepted presets require even coordinate spans within 4..512 by 4..256. Those
storage bounds are not a promise that every modified tiny preset generates a
playable map or supplies enough faction starting positions. Native asset-free
fixtures deliberately use a synthetic 8×8 preset; installed presets are above
that fixture size.

Custom coordinate spans must be even and within 32..512 by 16..256. There is no
rounding or area-based classification. The custom dialog uses the existing
integer conversion builtin; the native setter validates the converted integer
values. This differs deliberately from the original dialog's source-coordinate
input, numeric-prefix clamping, warning above 128, and admission of odd Y. The
original missing-section Standard fallback is not implemented: missing or
unsupported source fails preparation without publishing any frozen profiles.
These are declared compatibility limits, not claims about the original's
malformed-input rejection behavior.

## Wire and restoration boundary

The owned profile and accepted-selection codecs each have version 1. MapSettings
now has a typed version-1 envelope with optional acceptance. GlobalSettings is
version 5 and carries the complete host-frozen size profile for SMAC; absence
is required for SMAX. Initial lobby settings may carry no accepted selection.
Decode stages fields and validates the complete profile, category, dimensions,
locked equality and trailing frames before updating live map/settings values.
It preserves existing script wrappers and the local lifecycle lock. This is not
a transaction for a malformed later faction body in `State`.

WorldSnapshotRules version 6 adds explicit size-profile presence, the existing
complete profile codec and the accepted selection codec after the technology
profile. It requires exact equality with frozen session settings, including all
five raw names/dimensions and the accepted category. Old/unknown global, map and
world versions reject; mixed-version sessions cannot join through these codecs.
No stable disk-save compatibility is claimed.

After the header validates, the actual download decoder checks the existing
Tiles body's width/height frames against the accepted selection **before**
allocating the candidate Map. The actual writer checks its current map geometry
against that selection too. The shared body restore path performs the same
prefix check. This proves size agreement and leaves other body validation to
the existing codecs; it does not make malformed whole-world restoration atomic.
RNG, turn/completion, manager reconstruction and late publication ordering remain
unchanged.

## Validation

The native suite exercises actual SMAC/SMAX menu scripts with lightweight UI
factories and native settings, preset closure identity, custom cancellation and
rejection, actual `game_settings` apply/rollback and queued Event dispatch, and
State/Packet transport. The existing real Client callback tests now verify the
accepted selection is available at publication and reject malformed/changed
late size-profile fields without callbacks or settings/faction publication.
Immediate and queued actual final-chunk paths reject mismatched profile/category
and tile dimension frames before candidate allocation. Existing actual writer →
body decoder → InitComplete checks retain deterministic full reserialization.
These are headless integration tests, not rendered menu or multiplayer smoke.

The optional Release probe reads only an explicit installed source:

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Isrc src/game/backend/settings/SmacWorldSizeRules.cpp \
  src/loader/txt/SmacRulesTXTLoader.cpp src/loader/txt/TXTLoader.cpp \
  src/util/String.cpp tools/tests/smac_world_size_probe.cpp -o /tmp/world-size-probe
/tmp/world-size-probe /private/alpha.txt
```

It stages output and prints category, raw name, source X/Y and accepted coordinate
width/height. No proprietary source is distributed. The installed SMAC source
SHA-256 is `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
Local validation used Linux/WSL x86-64, Clang/LLVM 15.0.7 and source parent
`445ee2094331babdab7e82528eb00f028a65ff26`:

- Full normal CTest passed 8/8 in 51.90 seconds, including the expanded real CLI
  checks. Full ASan/UBSan/LSan CTest passed 8/8 in 73.30 seconds with
  `detect_leaks=1` and the existing local non-PIE sanitizer configuration.
- Subsequent test-only strengthening added valid-but-different preset/custom
  download records. The final selected native cases passed again with allocation
  tracking in normal and sanitizer binaries, including all four immediate/queued
  SMAC/SMAX final-chunk cases. No tracked memory/OpenGL leaks were reported.
- Eight changed production translation units passed the repository correctness
  policy with zero failures in 19.04 seconds. This is focused local analysis;
  shared-header CI selection remains unchanged.
- The Release production parser returned all five installed rows and their exact
  expected coordinate spans. Six malformed-count/number/layout/unsupported-height
  controls returned exit 1 without partial output.
- Three separately copied production translation units removed X doubling,
  changed custom 99 to a preset, or disabled snapshot selection equality. Each
  native test exited 1 at the intended assertion. Intended checkout source was
  not replaced by those controls.
- Final real menu/lobby/Event cases passed normal and sanitizer allocation-tracked
  runs after the duplicate/mixed-change guard and display-reversion refinement.
  Removing the duplicate guard in a separate script tree failed the actual
  handler regression at its intended assertion.

Initial test runs caught a staged-map assignment recursion through
`Serializable::operator=`, an absent Engine game service in the headless Player
callback fixture, and old fixtures that had not selected a size. Those were
corrected before the final receipts. Evidence is retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/world-size-*`.
No rendered UI or hardware-platform smoke is claimed by these local receipts.

### Focused publication

The initial focused publication is based on world-technology PR #116 at
`97eacd430e51c7df45e88e1ce2defed3fca6b631`. Applying source
`fe87bb2ba788d5a9c29b23b08a43d67e8de80c24` required only test-registration unions
and an explicit `value/Bool.h` test include; production behavior is unchanged.
That actual publication tree passed all six registered CTests in 34.57 seconds
(full GSE 24.15 seconds), including the new size/menu/packet/download cases.
This narrower parent does not contain all eight integration checks or imply the
source parent's sanitizer/lifecycle coverage. Its separately identified async
timer cancellation prerequisite must also be present for reliable hosted runs.

The final publication base is
`bebc643847291dc5eaed6bb05c3bc081105f55ed`: a history-preserving merge of that
PR #116 head with refreshed timer PR #69 at
`a5c3198d030a13fb5de6691c5f85386f804ca416`, which already includes stable main.
The independently reviewed conflict resolutions retain both parents' test and
include registrations. The timer production/tests are byte-identical to #69;
the resulting PR delta remains world-size-only.
On actual publication code head `505379e292c3718af193fecf90c5a9ff9edde4d4`, all six
registered CTests passed in 31.56 seconds (full GSE 21.08 seconds), including the
eight deterministic timer regressions. All eight changed production translation
units passed focused correctness analysis with zero failures in 16.08 seconds.
These checks use the publication tree itself; the earlier source-parent
sanitizer receipts above remain separately scoped.
