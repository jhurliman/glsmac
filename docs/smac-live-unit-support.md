# Conventional home Bases and mineral support

Conventional Units now own an optional stable home-Base ID alongside the
existing explicit Home discriminator. Ordinary native and GSE creation accept
`home: "base"` plus `home_base_id` only when that ID names an existing canonical
Base in this world with the same bound owner. Validation precedes Unit ID/tile
publication. Claimed new-world starting Units remain explicitly homeless; no
founding assignment or reassignment rule is inferred. Updating a Unit cannot
silently replace its home. Despawning a Base with existing home links rejects
until an explicit reassignment mechanism exists; reset teardown remains separate.

The canonical Base context owns `drone_riots`. Its existing checked scalar
replacement and detached getter preserve the value. First-base creation supplies
false as an explicit GLSMAC new-campaign policy; this change does not claim a
recovered original riot-reset producer. Restore preserves the saved value.
Homed morale queries use that actual status plus the registered facility at
source position 2 (Children's Creche). Missing canonical Base context or unbound
facility identity rejects. Queries never replace a home with a fabricated
homeless context. The existing original-backed morale kernel handles its mode
and modifier ordering. A riots-status change preflights future homed stats and
stages Unit/Base refresh queues and Base callback registration before publishing
the status; governor-only changes do not refresh Units. Citizen calculation and
facility-change producers remain separate callers.

`Game::GetCanonicalMineralSupport(Base)` consumes the actual registered Unit
population, checked current designs, saved home links, evaluating Base population
and its owner's retained SUPPORT. It delegates to `SmacUnitSupport` and returns
only total mineral demand. The admitted design population is still ordinary
projectile Scout Infantry and the exact selected Spartan Rover; unsupported
legacy units, packages or abilities reject. All admitted designs are non-psi
and have no Clean ability, so native terrain exemptions cannot apply. The
traversal order is a port map order, and only the order-invariant total is exposed;
no original free/paid per-unit flag or unit-array chronology is claimed. This
query is exposed through the authenticated Base `get_smac_mineral_support()`
getter. It does not consume RNG, alter Unit/Base state, account for convoys, or charge
support by reducing a tile yield. The live allocator can consume this total in
its existing support input in a separate integration.

World V22 rejects prior world layouts before body mutation. Unit manager and
Unit records are V3; Base manager is V8, with Base context V2. The latter's maximum
frame is 91 bytes and retains all prior fields plus the typed riots Boolean.
Simulation V8 and its satellite/RNG state are unchanged. A downloaded homed
Unit retains a transient unresolved-home flag because Unit records precede Base
records. It can own its restored identity/tile without exposing derived stats,
commands, frontend updates or a new snapshot. After both managers register the
saved records, actual InitComplete preflights every pending-home Unit against
the restored current simulation and Base registry, then clears all pending flags
without callbacks. Early raw starting Units remain under their independent
first-turn readiness seal. A missing or foreign link fails before any pending Unit is
released. The ordinary startup failure/reset machinery remains responsible for
retiring a failed world; this is not a manager-wide transactional restore claim.

## Evidence and checks

The arithmetic/selection source is the existing
[unit support receipt](behavior/unit-support-and-convoys.md), through the unchanged
[owned support kernel](smac-unit-support.md). Home morale uses the existing
[conventional stats](smac-conventional-stats.md) contract. New tests exercise port
ownership and persistence, not additional original-game execution.

Five focused groups cover stable-ID codec shapes; real native/GSE creation and
pre-effect missing/foreign/legacy-home rejection; actual Creche and saved riots
mode differences; live support totals with homogeneous, other-home, foreign and
homeless Units; malformed late home references with all-unit readiness retained;
and actual Start restore before/after Base registration. Existing conventional
Unit/Rover, Base context/restore, 28 original support outputs and 23 Clean
transitions, and orbital tests run alongside them. The invalid-reference test
rewrites the actual Unit frame while preserving other world frames, so it must
reach post-Base reference validation rather than an unrelated framing rejection.

Source `55e16d3d` passed the same 32 focused native groups with allocation
tracking in normal (1.066 s) and ASan/UBSan/LSan (3.121 s) builds, including the
eight claimed Early Start regressions. LLVM 15 correctness analysis passed all
nine changed production translation units (22.93 s, zero failures). Initial
fixture framing mistakes and the missing Space include were corrected before
these final runs; the parent review also caught the all-Units restore preflight,
which now correctly leaves Early startup under its own seal. No full suite,
platform matrix, original emulator rerun or default gameplay activation is part
of this slice.
