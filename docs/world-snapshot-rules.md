# World-download rules binding

Multiplayer world downloads now start with a separate, typed rules header:
integer marker `0x474c53574f524c44` (`GLSWORLD`), integer version `5`, the exact
`smac` or `smax` identity, and a Boolean indicating SMAC numeric rules. SMAC then
includes nine signed-int32 values in Base Square, Bonus Square, Recycling Tanks
order, with nutrients/minerals/energy inside each row. A second Boolean declares
the production profile's presence. SMAC requires it and follows it with one
String frame containing the existing versioned `SmacProductionRules` codec;
SMAX requires absence and has no production payload. A third Boolean declares
the technology catalog's presence, followed for SMAC by one String frame with
the existing versioned `SmacTechnologyRules` codec. SMAX requires absence here
as well; it does not inherit a SMAC catalog.

The [frozen production profile](smac-production-profile.md) includes all four
production/retool constants and the complete 71-slot facility/project catalog,
including each source ID, display name, signed cost/maintenance and exact
prerequisite token. World encoding and decoding call that codec directly;
they do not maintain another copy of its field ordering. Decoded profile
equality compares every modeled field against the established frozen session.

The [frozen technology catalog](smac-technology-catalog.md) contains all 89
positional records, including identity text, seven-byte keys, the four raw
priority positions, both prerequisite tokens and flags. Its independent V1
codec performs structural validation before exact catalog equality. The header
does not interpret ownership, grant a technology, or decide buildability.

Both writing and validation require a frozen session profile. The client
compares the header against the host profile already accepted through global
settings. Validation neither changes those settings nor reads local rules
files. Unknown markers/versions/identities, legacy string-first downloads,
inconsistent presence, out-of-range numbers, truncation, and any differing
numeric, production or technology value fail before map allocation. This includes invalid
or mismatched fields at the end of the catalog, not merely the scalar constants.
The writer validates/encodes the complete profile before appending any header
fields. Allocation failures do not promise transactional Buffer writes.

The actual Game download handler validates through `WithWorldIngress`, where the
callback may execute later on the accumulation thread. Native format errors are
caught there and request disconnection with `Snapshot format mismatch`. Catching
only in the packet dispatcher would miss deferred failures. Client completion
holds a local copy of its callable while invoking it because the Game handler
clears the installed callbacks as it starts.

Version 5 follows the profiles with the unchanged [backend RNG continuation](world-snapshot-rng.md)
and [turn/completion block](world-snapshot-turn.md). Versions 4, 3, 2, 1 and unversioned
downloads are unsupported. After that prefix, the map/resource/unit/base/animation
frames keep their previous order and bytes; the former trailing turn field is
now in the prefix. The header codec consumes only the header; it
does not treat body bytes as extra header fields. This change does not version
map files, add a complete save format, make body restoration transactional,
implement transfer-size limits, or activate yield/production gameplay consumers.

Native tests use synthetic SMAC values including negative numbers and both
int32 endpoints. They cover all nine mismatches/range failures, frozen-profile
requirements, every modeled production and technology field, SMAX absence, malformed nested
profiles/headers, and preservation of trailing bytes. A real Game writer test
compares the entire remaining body with the previous frame layout. Real Client
final-chunk dispatch invokes the installed
Game callback both immediately and from a worker thread, then processes queued
accumulations. Those download fixtures preload frozen settings directly; the
separate host-rules tests cover the real State/Packet settings handshake.
Rejection preserves map absence (or the existing map and its bytes for the
technology cases), manager bytes, settings, RNG, turn/completion,
frontend requests and the pre-initialization state, without invoking start,
turn, unit-spawn or base-spawn callbacks. SMAC malformed/mismatched profiles and
an unexpected SMAX profile both traverse immediate and queued final chunks.
Technology cases include malformed final records, missing presence/payload,
wrong frame types, out-of-range metadata, trailing fields, and truncation.
Matching-header cases with a missing or wrong-type map frame also exercise the
deferred native-error handler. These asset-free checks stop before successful rendered-map creation;
they do not claim a successful rendered Client download or original-game
runtime observation. A separate positive fixture uses the actual world writer,
header validator, body decoder and `InitComplete` with its existing asset-free
Map. Both SMAC and SMAX fixtures reserialize to the identical complete snapshot
after installation, covering map/manager bytes, the frozen profile, RNG and
turn/completion state. That does not turn the fixture's skipped asset-bearing
constructor/render initialization into a tested path.

Run the normal native suite with `ctest --test-dir build/host-rules
--output-on-failure` (or the equivalent configured build directory). See
[host rules](smac-host-rules.md) for the earlier settings/file-source boundary.

The initial rules-header validation on Ubuntu/WSL with Clang 15 passed all six CTest entries
(173 native/script cases) and the full `--memorydebug` suite. Focused clang-tidy
checks passed for the four changed production translation units. Disabling
numeric equality made the mismatch regression fail; removing the inner native
error boundary made both immediate and queued completion regressions fail.
Both guards were restored before final validation.

The first ASan+UBSan run passed the gameplay/script suite but a CLI subprocess
segfaulted. A `--help` reproduction under GDB stopped in sanitizer address-space
initialization before `main`. Appending `-no-pie` to that isolated build's
existing linker flags avoided this known local startup failure; the final
sanitizer CTest run passed all six entries. This was a local cache workaround,
not a repository toolchain change or a clean first-pass sanitizer claim.

## Download fixture scheduling

The fixture finishes setup accumulation before dispatching the final packet,
joining a worker, or draining queued callbacks. The runtime-rooted State keeps
its attached Client reachable across these phases; cleanup resets handlers and
detaches the client inside a final accumulation. Protocol/state assertions still
exercise the real immediate and deferred completion paths.

This ordering avoids a test lock cycle: accumulation holds the collection lock,
collection takes the pending-work lock before the collection lock, and a worker
needs the pending-work lock to enqueue its callback. Joining that worker from
inside accumulation can therefore stall all three threads. The fixture now
checks its outside-accumulation precondition before dispatch and drain. No
production collector lock behavior changes.

A deterministic negative control restores the old outer accumulation around
dispatch. Both download tests then fail promptly on the precondition, before
spawning a worker, instead of depending on a scheduling-sensitive timeout.

After restoring the corrected dispatch placement, local Clang 15 / WSL CTest
passed all seven entries (35.92 seconds). The negative run finished with 203
passes and the two expected download-test failures, without a timeout.

## Version 4 validation

The V4 delta was validated on the integrated source at
`23dcaacfc4811803481817e8990ace8e8b7cdc14`, using Clang 15 on Ubuntu/WSL.
The normal CTest suite passed all eight entries in 43.18 seconds. Sixteen focused
native groups passed with allocation tracking; the same selection passed under
ASan, UBSan and LSan (`detect_leaks=1`) with allocation tracking and no reported
memory/OpenGL leaks. Debug and Release correctness analysis of
`WorldSnapshotRules.cpp` each passed with zero findings (0.85 seconds each).

The pure mismatch group checks 288 valid differences: four scalar constants
and four mutable metadata fields for each of 71 catalog slots. Source IDs are
fixed positional identities and malformed IDs are tested separately. Running
that group against the prior production header failed on the first different
production rule; the fixed reader rejects every difference. The real immediate
and queued Client regressions additionally check malformed late fields and
SMAX's absent-profile contract without setup callbacks or world mutation.
Their negative headers intentionally stop at the rejected prefix; they do not
stand in for a complete valid rendered download. Positive asset-free body
restoration asserts byte-for-byte equality of the complete reserialized world
snapshot for both rulesets. These checks do not establish rendered Client
startup, disk save compatibility, or future multiplayer lockstep.

## Version 5 validation

The V5 source delta is based on integrated technology source
`ea35b6cb95e6564516440379414eefc4822900e1`, with the Windows enum-name correction
`77aa41bec751a0d507bb9e4cbbcd7d460cd474e6` applied separately. The normal
Linux/WSL Clang 15 suite passed all eight CTests in 50.74 seconds. After the
enum correction, the focused header/writer/real-Client selection passed again
with allocation tracking. The full ASan/UBSan/LSan suite passed eight of eight
in 61.20 seconds (`detect_leaks=1`); its focused allocation-tracked selection
also passed with no reported memory/OpenGL leaks. The local sanitizer build
retains the existing non-PIE startup workaround.

Combined correctness analysis, including the technology catalog's shared
settings header, checked all 307 Linux production translation units with
LLVM 15.0.7 and four jobs: zero failures in 115.05 seconds. The existing
Windows-only translation-unit exclusion is unchanged. Two independently
compiled source-copy controls each exited one at the expected assertion:
disabling technology equality admitted a valid but different catalog, and
removing the nested codec's trailing-field check admitted an extra field.
No intended checkout source was replaced for those controls.

The pure equality matrix checks 801 valid differences (nine modeled fields
per record across all 89 records). Malformed positional IDs are checked
separately. The immediate and queued real Client tests retain an existing
asset-free map for technology failures and verify its pointer and bytes remain
unchanged, alongside settings, managers, RNG, turn/completion state and callback
counts. This proves rejection before body mutation, not rollback after a
malformed body. Local receipts are retained outside the checkout under
`/home/jhurliman/src/glsmac-world-technology-audit`.
