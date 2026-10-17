# Owned roster value records (inactive)

`game/backend/RosterRecords.h` provides owned Player, Slot and Roster metadata
records. No packet handler, existing serializer, native Player, Slot, Faction,
State, callback or world snapshot uses this format yet. This is a prerequisite
for a reviewed canonical roster publication change, not a lifecycle fix or live
research state. It has no proprietary data or original-game format claim.

Player metadata contains an exact name, one of Single/Host/Player, an optional
faction ID and an exact difficulty ID. A missing faction is deliberately distinct
from a present empty ID. IDs are unresolved: decoding does not allocate Factions,
look up definitions, grant technologies or choose defaults.

Slots are ordered values, with Closed/Open/Occupied state. Only occupied slots
have a Player, a nonempty linked GSID and flags (the existing three bits: ready,
map downloaded, game initialized). Vacant slots have no Player, flags or GSID.
A roster rejects duplicate occupied GSIDs. Duplicate display names and faction
IDs are permitted. The codec does not enforce host slot position, authentication,
occupant continuity, readiness transition rules or a fixed live session size.
An empty roster is structurally valid; a live publication policy may reject it.

Names and IDs are nonempty, NUL-free byte strings. Case, whitespace and UTF-8 byte
spelling are preserved without normalization. Vacant GSID is the sole empty text
field. These are explicit new-protocol admission rules; they do not describe
every value the legacy serializers currently accept.

## Wire and bounds

The new standalone envelopes are independent of `types::Buffer` and its existing
live wire formats. All 32-bit words are unsigned little endian. One byte encodes
roles, states, flags and Boolean presence (only 0/1). Text is a length word followed
by that many bytes. Nested records likewise have a length word and exact envelope.

| Kind | Four-byte marker | Fields after the version word (`1`) |
| --- | --- | --- |
| Player | `GLPR` | name, role, faction-presence, optional faction ID, difficulty ID |
| Slot | `GLSL` | state, Player-presence, optional nested Player, flags, linked GSID |
| Roster | `GLRS` | slot count, ordered nested Slots |

Each decode takes an immutable byte view and returns a complete owned value.
Every envelope rejects the wrong kind/version and trailing bytes, including
inside an otherwise valid parent. Encoding validates the same value contract.
There is no checksum or authenticity claim; a future transport must supply its
own framing and trust policy.

The caller must supply `Limits {max_slots, max_text_bytes, max_encoded_bytes}`.
All three must be positive and fit uint32. No defaults or game limits are chosen
here. Text budget is the **sum** of all name, difficulty, faction and GSID bytes
across the complete supplied record. Encoded budget counts the entire envelope,
including all nested bytes exactly once. Exact budget endpoints are accepted.

Decode first checks the total encoded size, then performs a validation pass over
views with no allocation of decoded values. It checks each extent by subtracting
from remaining bytes, validates shapes, and accounts for aggregate text. A count
must fit both the caller's slot limit and the remaining four-byte entry prefixes
before it can guide allocation. Only then does a second pass create owned values.
Duplicate GSID detection happens after this bounded materialization; failure
destroys the local result. Error-message allocation and allocation failure are
not promised to be allocation-free. Encoding measures all nested output lengths
before allocating the output string. Input bytes/native values never change;
`destination = Decode...()` leaves destination unchanged on rejection.

## Validation

Seven native regression groups are registered in the ordinary GSE suite. They
cover independent literal wire bytes, all roles/flag combinations, absent versus
selected factions, ordered open/occupied/closed slots, source/copy/move ownership,
every truncated prefix of three record shapes, nested trailing data, hostile
uint32 lengths/counts, malformed enums/presence/shape, NUL/empty text, duplicate
GSIDs and exact aggregate limits. The source has no live packet path to use as an
old-code negative control: copied production mutations exercise failure detection
without changing the intended source.

Measured validation and immutable source provenance are recorded with the commit
receipt below. The first test run exposed one extra zero byte in the independent
vacant-Slot golden fixture (six groups passed, one failed). The fixture was fixed;
the initial failure log is retained. It was not a production-code defect.

Source validation on the integration parent
`445ee2094331babdab7e82528eb00f028a65ff26`, WSL Ubuntu / Clang 15:

- Full normal CTest: 8/8, 49.28 seconds. Seven focused groups with memory tracking:
  1.128 seconds, no tracked leaks.
- Seven focused groups with ASan/UBSan/LSan and memory tracking: 1.088 seconds,
  leak detection enabled, no errors. This is focused sanitizer coverage, not a
  claim of a new full sanitizer-suite run. The existing local sanitizer build
  uses `-no-pie` for the separately recorded WSL pre-main startup problem.
- Copied production controls: omitted trailing checks failed two groups; reset
  aggregate text accounting failed one; relaxed role validation failed one;
  copied the name into difficulty failed three. Each exited 1. Intended source
  was never replaced by a mutant.
- A standalone `-O3 -DNDEBUG -Wall -Wextra -Werror` production probe passed 24
  round trips and 2,064 truncated/budget/trailing rejection cases. Explicit
  correctness analysis of the production TU passed in Debug (3.87 seconds) and
  a Release compilation database (3.64 seconds), zero failed TUs.

Commands, source copies, initial fixture failure, control binaries/logs and
manifests are retained locally in
`/home/jhurliman/src/glsmac-roster-value-codecs/`. No installed assets are needed.
No Windows/macOS execution or hosted CI result is inferred from these receipts.

Focused publication on main `dab5a476bb93dd0d3acf2f9380933749d8402dcb`
retains the exact codec and dedicated tests from source
`d6b045d3ece3589640dac495055120b06248ba9b`; only registration context differs.
Its actual normal CTest passed 5/5 in 24.38 seconds, and the full-suite log
contains all seven new groups. This older main does not have the native-filter
CLI. A local copied `Tests.cpp` that registers only these seven groups was linked
against the publication objects and passed with memory tracking in 1.094 seconds,
with no leaks. This registration-only diagnostic is not a committed CLI change
or a full memory-tracking-suite claim. Publication commands and logs are retained
in `/home/jhurliman/src/glsmac-roster-value-codecs-publication/`.

## Next coherent live boundary (not activated here)

The approved follow-up requires all of these together before replacing live
packet handling:

- State-owned initial/refresh/single-slot publication, with all validation and
  fallible preparation before native pointer publication, followed by retirement
  under the existing native mutation exclusion. Keep transport, queue registration
  and callbacks outside it.
- Stable Slot addresses after the first fixed-size roster. Preserve a live Player
  only for the same slot, nonempty server-accepted GSID and role within one roster
  epoch. Vacancy ends identity, even if the next occupant has the same GSID.
- Canonical faction-ID lookup through a reset-invalidated roster identity. Game
  session identity is insufficient because startup rebinds it while retaining
  Players. No raw cached manager Faction or detached decoded Faction ownership.
- Stage the initial roster until valid host settings and factions exist. Preserve
  the actual initial packet order, joins before OnOpen, random-faction refresh,
  and running reconnect. Existing partial-settings-decode failure remains an
  explicit boundary; no whole-settings transaction is promised by this slice.
- Owned read-only ephemeral notification values, exact occupant checks for queued
  updates, State/Connection roots through registration and execution, and cleanup
  on immediate exceptions or queued cancellation. No shallow Slot/Faction copies
  or notification retargeting after same-State reset.

The codec requires only C++17, the existing native test harness, and registration
list unions for publication. The live integration requires the reviewed native
mutation, initialization coordinator and departure-retirement prerequisites, with
actual Client packet/collector tests. This slice does not close the held PR77
findings or define initial research, discoveries, costs or progression.
