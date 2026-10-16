# Authoritative base production state (V4)

Each Base owns an optional production record. A newly constructed base is
**unconfigured**; the port does not invent an initial target or retool baseline.
A configured record contains four independent values:

| Field | Storage |
| --- | --- |
| `current_item` | Owned tagged identity |
| `minerals` | Signed 64-bit integer |
| `baseline_item` | Owned tagged identity |
| `baseline_minerals` | Signed 64-bit integer |

An item has a `kind` of `unit`, `facility` (local base facility), `orbital`,
`project`, or `stockpile`. The first four require a nonempty owned string `id`.
Stockpile has no catalog identity. Tags distinguish equal strings belonging to
different kinds. This is structural persistence, with no definition pointers or
catalog lookup: preserving an unresolved identity does **not** make it buildable.
A future execution resolver must explicitly reject missing/ineligible definitions.

Signed 64-bit balances are a port storage-domain choice, including negative values
and both endpoints. They are not a claim that original signed 32-bit arithmetic is
faithful outside its supported domain. Future cost, posting, and switching
calculations must validate inputs and intermediate results separately.

## Script contract

`base.get_production_state()` returns `undefined` for an unconfigured base, or a
fresh object with the four fields above. Each non-stockpile item is a fresh
`{kind, id}` object; stockpile is `{kind: 'stockpile'}`. Neither changing the
returned object nor retaining and mutating setter input changes native state.

`base.set_production_state(record)` accepts exactly those four fields, integer
balances, and structurally valid item objects. It rejects unknown/additional
fields, missing fields, wrong types, unknown kinds, empty identities, and identities
on stockpile. `undefined` explicitly removes the complete record; `null` is not a
second clearing syntax. The setter requires a game write scope before validating
its argument, validates and copies the whole replacement before mutation, and
triggers one coalescible `update_base` notification on change. An identical record
does not trigger an update. There is no automatic change to the baseline when the
current item or mineral balance changes.

## Snapshot contract

The base-manager format is `GLSMAC_BASES_V4`. V3 and older/unversioned/unknown
formats are rejected; there is no migration. Each base appends a framed production
record after accumulated nutrients. The record has a typed presence boolean and,
when present, current kind/identity/balance and baseline kind/identity/balance.
The decoder validates tags, identity structure, field types, truncation, and
trailing production data before constructing the Base, claiming its center,
advancing its ID, or publishing its workers. Immediate and deferred manager
restoration use the same decoder.

This is per-record publication safety. A manager load can already have restored
definitions or earlier valid bases when a later record fails; such a failed load
remains terminal. This change does not make an entire snapshot transactional.

## Evidence and limits

The schema separates fields because the SMAC evidence handoffs
`RE-SMAC-RETOOL-001`, `RE-SMAC-PRODUCTION-POST-001`, and `RE-SMAC-QUEUE-001`
describe distinct writers and timing. In particular, switching checkpoints may
change the current target/balance while preserving the old baseline target/balance;
posting may update the baseline balance before a later writer updates its target;
queue promotion is separate again. Tests store the supplied A/30 → B/20 → C/20 →
A/30 checkpoints with an unchanged A/30 baseline, plus a distinct 9/30 balance.
These tests verify storage independence, not an implementation or original-runtime
comparison of those algorithms. No proprietary code or assets are required.

This slice does not implement production selection, retool penalties, queues,
eligibility, mineral posting, cost factors, unit/facility/project completion,
Stockpile income, or the existing placeholder production UI. It makes no new SMAX
gameplay claim. The event regression uses a synthetic registered handler through
the real optimistic event/response dispatcher to prove that complete state can be
returned and restored on rejection; it is not a production scheduling event.

## Validation

Native tests beginning `test base production` cover unconfigured/configured
immediate and deferred snapshots, every tag, signed endpoints, independent
baselines, unchanged populations/nutrients/worker links, copied getter/setter
values, read-only rejection, atomic malformed setters, malformed first/later
records, and real optimistic event rollback. The existing version-rejection test
also rejects V3 before definitions or bases are published.

Run the focused suite with `GLSMAC --gse-tests --gse-tests-native-filter
'^test base production' --memorydebug` (or the complete `--gse-tests` suite on a
publication parent without the native-filter CLI).

Source-branch validation used Clang 15 Debug under WSL, based on
`f4f7958172131dc2336fbd333884b4119fb0f9a1`: all six focused cases passed with
allocation tracking, normal full CTest passed 8/8, and ASan/UBSan/LSan with
`detect_leaks=1` passed the six tracked cases and full CTest 8/8. The existing
local non-PIE sanitizer linker workaround was retained. The full LLVM 15
correctness policy analyzed 301 production translation units in 58.38 seconds
with zero failures; the Windows-only TU remains outside that Linux policy.
These are local source-branch results, not hosted or native Windows/macOS results.

Two separately compiled production-code copies each produced two deterministic
test failures (four other focused cases passed), exit 1: conflating the serialized
baseline with the current item/balance, and accepting trailing production-record
data. Intended source was never replaced. Logs and control copies are retained
under `build/owner-leaks/validation/production-*` in the managed source tree and
`/home/jhurliman/src/glsmac-production-state-audit/`. An initial event fixture
omitted synthetic render coordinates required by `Game::Iterate`; its failure
logs are retained separately and are not counted as a production defect.
