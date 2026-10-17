# World-download rules binding

Multiplayer world downloads now start with a separate, typed rules header:
integer marker `0x474c53574f524c44` (`GLSWORLD`), integer version `3`, the exact
`smac` or `smax` identity, and a Boolean indicating SMAC numeric rules. SMAC then
includes nine signed-int32 values in Base Square, Bonus Square, Recycling Tanks
order, with nutrients/minerals/energy inside each row. SMAX has no SMAC payload.

Both writing and validation require a frozen session profile. The client
compares the header against the host profile already accepted through global
settings. Validation neither changes those settings nor reads local rules
files. Unknown markers/versions/identities, legacy string-first downloads,
inconsistent presence, out-of-range numbers, truncation, and any differing
numeric value fail before map allocation.

The actual Game download handler validates inside `State::WithGSE`, where the
callback may execute later on the accumulation thread. Native format errors are
caught there and request disconnection with `Snapshot format mismatch`. Catching
only in the packet dispatcher would miss deferred failures. Client completion
holds a local copy of its callable while invoking it because the Game handler
clears the installed callbacks as it starts.

Version 3 follows the profile with the [backend RNG continuation](world-snapshot-rng.md)
and [turn/completion block](world-snapshot-turn.md). Versions 2, 1 and unversioned
downloads are unsupported. After that prefix, the map/resource/unit/base/animation
frames keep their previous order and bytes; the former trailing turn field is
now in the prefix. The header codec consumes only the header; it
does not treat body bytes as extra header fields. This change does not version
map files, add a complete save format, make body restoration transactional,
implement transfer-size limits, or activate the SMAC yield calculator.

Native tests use synthetic SMAC values including negative numbers and both
int32 endpoints. They cover all nine mismatches/range failures, frozen-profile
requirements, SMAX absence, malformed headers, and preservation of trailing
bytes. A real Game writer test compares the entire remaining body with the
previous frame layout. Real Client final-chunk dispatch invokes the installed
Game callback both immediately and from a worker thread, then processes queued
accumulations. Those download fixtures preload frozen settings directly; the
separate host-rules tests cover the real State/Packet settings handshake.
Rejection preserves map absence, manager bytes, settings, turn,
frontend requests and the pre-initialization state. Matching-header cases with
a missing or wrong-type map frame also exercise the deferred native-error
handler. These asset-free checks stop before successful rendered-map creation;
they do not claim a full world restoration or original-game runtime observation.

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
