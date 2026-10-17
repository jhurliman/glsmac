# Owned live roster publication

The client and server now use the versioned owned records from
[roster-value-records.md](roster-value-records.md) for their Player/Slot metadata.
Previously, slot decoding created Players without consistently adopting them
into State, full roster refresh replaced their addresses, and each Player
decoded its own Faction allocation. The new native publication boundary keeps
one State-owned Player per occupied Slot and resolves its selected faction from
the current canonical FactionManager.

This is protocol and lifetime integration, not research gameplay. No technology
possession, progress, faction starting grants, or other research defaults are
introduced. The preserved turn-completed field is a regression sentinel for
non-wire Player state that continuous metadata updates must retain.

## Identity and supported session policy

Initial publication creates exactly seven Slots. Their addresses and count stay
fixed until roster retirement. An occupied-to-occupied update preserves the
Player and its wrappers only for the same slot, nonempty GSID, and role in the
same roster epoch. A changed identity rejects until an actual vacancy. Vacancy
invalidates the old Player identity and linked wrappers; rejoining with the same
GSID creates a fresh Player. This is an explicit continuity rule, not proof of
authenticated account ownership. Lobby slot indices 0..6 are not the original
simulation faction IDs 1..7. No custom design ID or simulation faction identity
is inferred from a slot index, faction template or display name.

A roster epoch is separate from a Game session: startup may rebind the Game
without replacing its lobby Players. Reset retires the epoch before deleting
Players and clears a detached Connection's cached local Player binding. Decoded
update calls carry the captured epoch into the guarded commit;
reusing the same State and Connection objects cannot make an old update current.

The live codec policy is seven slots, 32 KiB of aggregate text and 48 KiB of
encoded roster data. Both exporters and decoders use it. Server authentication
validates the proposed aggregate before adopting another Player. A native test
fills the aggregate text budget, serializes the real PT_PLAYERS envelope, and
checks that it plus the transport length prefix fits SimpleTCP's 65,536-byte
buffer. These are port limits, not claims about every original-game or modded
roster. The supported desktop targets use 64-bit size_t, retaining the full
received flags integer until the checked 0..7 admission step.

## Initial handshake and canonical factions

Server packet order remains PT_PLAYERS, PT_GAME_STATE, PT_GLOBAL_SETTINGS.
The client first stages owned metadata and the assigned local slot. It verifies
the local GSID, then waits for valid settings and faction definitions before
publishing any Players or opening the configured connection. Initial publication
requires Host in slot zero, a nonzero occupied local Player slot, valid roles,
known difficulties, unique GSIDs, and resolution of every selected faction ID.

An absent faction ID remains the existing random/unselected choice. A present
missing ID is an error. Players retain the ID and a checked State/epoch binding,
not an independently allocated or cached Faction pointer. Same-ID manager
replacement therefore resolves the new canonical object. This supports the
authoritative random-faction refresh before startup without replacing ready
Players or duplicating join notifications. Running disconnect/reconnect keeps
the same Player and native state.

The settings decoder retains its existing partial-failure behavior: completed
settings or faction records may remain after a later field fails. An invalid
initial handshake still leaves its roster unpublished. A selected faction
missing after a later failed settings replacement reports a lookup error; it
does not silently become random. This is not a whole-settings transaction.

## Native commit and notifications

Bytes decode into owned values before native publication. State's existing
collect-only `WithNativeMutation` scope checks the current Connection, exact
epoch, phase, identity, readiness and canonical references. It prepares fallible
owned allocations before changing pointers, then updates State membership,
Slot-to-Player and Player-to-Slot links, local Connection binding, and server
CID bookkeeping. Retired native Players are destroyed while collection is
excluded. No script callback, queue registration, transport operation or wait
runs inside this commit scope.

Notifications own scalar metadata and the captured epoch. A State handoff list
keeps their State/Connection references visible between native publication and
Space queue registration; the queued owner then supplies those roots. Dispatch
checks retirement before native access and materializes a read-only Player-like
value only inside owner accumulation. Its wrapper and retained methods expire
when the callback returns or throws. It has no Slot pointer and cannot serialize
as a live Player reference. Normal game Player wrappers remain writable through
the existing game mutation checks.

Entries are consumed before invoking handlers, so a throwing callback retained
by the existing Space queue is not replayed on the next drain. Native and GSE
errors leave an already committed roster committed. Subsequent packet callbacks
check the exact epoch/current Connection before continuing after synchronous
reset, including listen, ready clearing, initial publication, and slot updates.
Immediate callbacks complete before later native callbacks; on a foreign thread,
the existing queue preserves insertion order while native callbacks can run
before the owner drains the queue. Tests assert both actual connection-open
orders. Generic Connection::WTrigger and OnOpen are not redesigned here.

## Validation and limits

Twenty asset-free native groups in `src/gse/tests/LiveRoster.cpp` use real Client
packet decoding, real Server authentication/update/disconnect paths and the
Network MT event queue. They cover canonical initial publication, continuity,
vacancy/rejoin, ready random-faction refresh, running reconnect, strict rejection,
framing, stale exact-epoch commits, callback reentry, pending roots, scoped
notification lifetime, and native/GSE exception retry.

The collector test pauses an actual Player destructor notification after linked
wrapper invalidation while a real foreign-thread packet is committing vacancy.
A collector thread announces that it started, and a bounded 100 ms observation
checks that it cannot enter traversal during retirement. Cleanup always releases
the gates and joins workers. A callback-side collection checks that exclusion has
ended before native notifications. This is observed overlap evidence, not a
scheduler-fairness proof or an explanation of historical untraced timeouts.

Existing backend fixtures now establish real settings/roster readiness instead
of bypassing the handshake or serializing unowned legacy Players. Their startup,
snapshot identity, packet-batch and collector assertions remain present. Legacy
full Player/Slot/Slots metadata decode APIs reject direct use; live writers and
readers switch together. The separate slot-based GSE Player-reference format is
unchanged, apart from rejecting ephemeral notification references.

The final source passed the following Linux/WSL Clang 15 checks:

- All twenty focused native groups with `--memorydebug`: 1.09 seconds in both
  normal and sanitizer builds, with no tracked leaks.
- Normal CTest: 8/8 passed in 44.75 seconds.
- ASan/UBSan/LSan CTest: 8/8 passed in 60.41 seconds with
  `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- Existing LLVM 15 correctness policy over the nine changed production
  translation units: zero failures, 10.82 seconds. The final Client-only
  captured-epoch consistency change was rechecked separately: zero failures,
  2.11 seconds.

The focused command is:

```sh
GLSMAC --gse-tests --gse-tests-native-filter '^live roster ' --memorydebug \
  --prefix /path/to/a/dedicated/test-prefix
ctest --test-dir /path/to/existing/build --output-on-failure
```

Seven independently compiled copies of production translation units each failed
their targeted native group with exit 1: omitting retirement exclusion, resetting
non-wire Player state on metadata update, allowing an occupied GSID change,
replaying a consumed notification, omitting aggregate authentication admission,
validating a newly sampled epoch instead of the captured one, and treating a
missing selected faction as random. The aggregate-admission control first admits
an oversized native roster and then fails its subsequent real export with
`Roster record: budget exceeded`. These are mutation controls for the new
protocol, not executions of the incompatible old wire protocol. The retirement
control observes collection entering the paused destructor boundary; it does not
depend on allocator reuse or dereferencing retired storage.

Earlier failed runs are retained: old fixture handshakes and missing fixture
roots were corrected, along with a reused test context constant and an incorrect
assumption that State reset clears the FactionManager. One intermediate CTest
run hit a ruleset-CLI executable permission error because its binary was being
relinked concurrently; the final runs above had no build/test overlap. The local
sanitizer build retains the previously documented no-PIE diagnostic workaround;
no build policy was changed in this slice. These receipts do not claim Windows,
macOS, rendered multiplayer, or hosted review coverage.

Local commands, source hashes, copied controls, logs and receipts stay outside
the repository in `/home/jhurliman/src/glsmac-canonical-live-roster`; the final
index is `source-evidence-manifest.json`.

The source branch is based on `bbdd871273382a4202824a4a4853866a7589926f`.
It relies on the existing owned-record codec, native mutation exclusion,
callback/collector teardown fixes and initialization coordinator. Publication
onto an older branch must include those prerequisites and run its actual tests;
source-branch results do not validate an adapted publication automatically.

This does not synchronize arbitrary gameplay or lobby readers, make all native
roster methods safe from unrelated concurrent callers, redesign authentication,
make generic Connection callbacks cancellation-safe, permit live-world occupant
replacement, or resolve the held PR77 findings. Canonical lookup still requires
the caller's existing owner accumulation/native exclusion. No new GC locking API
or whole-State::Iterate lock is added.
