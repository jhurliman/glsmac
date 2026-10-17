# Supplied neutral Scout combat

`game::backend::unit::smac_scout` is a pure, owned model for the checked neutral
flat-land conventional Gun/Scout-versus-Gun/Scout context. It accepts resolved
inputs and returns a result without modifying its inputs or external state.
It does not create or activate live Units, choose enemies, query terrain or
technology, infer morale/allowances, admit UI commands, or update a world.

The prerequisite is [the two supplied gameplay RNG states](smac-gameplay-random-state.md).
The existing map JSF32 stream and its world continuation remain unchanged.

## Inputs and deliberate domain

`ResolveNeutralFlatAttack()` requires two distinct stable uint64 identities,
both supplied random states, an explicit attacker allowance, and explicit
optional selected/secondary references. Identity zero and UINT64_MAX are valid;
absence uses `std::optional`, not a sentinel identity or array index.

Each participant supplies reactor, remaining HP, stored morale, effective
morale, and spent movement. The admitted domain is:

- Reactor 1..4 and positive remaining HP at most `10 * reactor`.
- Stored and effective morale equal, each 0..6, for this neutral profile.
- Supplied attacker allowance 3 or 6. The model does not derive it from morale,
  reactor, damage, faction, or any default.
- Spent movement 0..255; attacker spent at most 252 so adding the full cost 3
  cannot wrap the original byte domain. Original wrapping at 253..255 is
  unverified and rejected, rather than silently clamped or widened into a
  compatibility claim.
- Selected and secondary references name either participant or are absent.

Invalid inputs throw `std::invalid_argument` before drawing or producing a
result. Arithmetic uses widened intermediates and checks its nonnegative
signed32 domain. Each hit removes one HP, so at most 79 rounds and 160 CRT draws
(including promotion) are possible. The transcript vectors are owned and
reserved to those bounds; results are never truncated to a limit.

This contract covers the supplied-world, neutral human-owner versus AI-owner,
Librarian difficulty, Vendetta, no-base/no-project/no-ability context of the
original captures. Other weapons, armor, Psi/natives, modified/home morale,
terrain, sensors, difficulty modifiers, artillery, collateral, retreat and
capture remain outside scope. Matching input numbers alone is not proof that
an arbitrary live context belongs to this profile. SMAX equivalence is not
established.

## Resolution and normal return

Attack strength is `32 * (6 + effective morale)`; defense uses its separate
morale floor of 1. The attacker's remaining movement scales only its strength
when less than 3 remains. Each round draws attacker then defender from CRT,
uses raw modulo width, suppresses draws at width <= 1, and gives ties to the
defender. Strength does not change as damage accumulates. There is no further
round draw after the lethal hit.

Promotion uses the recorded winner/loser weights and ordering. Its outer draw
can occur before an elite winner is refused by the helper. Lower morale can
skip the outer/helper draw; morale 3..5 uses the extra parity draw. The shared
stream is preserved. The result exposes every round and actual CRT transition.

`AttackResult::victory` is the typed resolved outcome. `original_returns` is
separate historical evidence: the core returns 1 for an attacker victory and
0 for a defender victory, while the movement wrapper returns **0 after either
kill**. That zero is not rejection or rollback. The survivor retains health,
promotion and spending; an attacker victory spends a full 3 even when fewer
than 3 points remained.

The survivor remains at logical `Source` after an attacker victory or
`Destination` after a defender victory. Its singleton stack links are absent,
the losing square's presence is false, and the dead identity is returned.
Selected/secondary references clear only when their participant dies. The
separate continuation identity names the survivor and never replaces a cleared
selected reference. No original compacted array index escapes this interface.
Actual map coordinates and live deletion/publication belong to a later adapter.

`ContinueEmptyFlatWithoutLottery()` is deliberately smaller than a general
movement command. It accepts the surviving singleton at `Source`, freshly
supplied allowance 3/6, and either at least 3 remaining points or exhaustion.
It performs the three checked post-kill histories: full-cost move into the now
empty destination, exhausted refusal, or overspent refusal. It makes no RNG
draw and preserves reference meaning. Remaining 1/2 is rejected because that
helper does not implement movement lotteries. This is **not** a claim that a
partial-movement keyboard command is illegal.

The separate RE-SMAC-SCOUT-KEY-001 handoff establishes that owned selected units
with spent below current allowance can emit their own identity, including
remaining 1/2; exhausted/overspent selected units do not. The battle model still
accepts the directly supplied exhausted hostile diagnostic requests from its
combat corpus. A future command adapter must implement that separate admission
boundary, rather than treating this diagnostic model as the UI policy.

## Original fixtures

The human-owned behavior files are read-only evidence inputs:

| Research record | Case JSON SHA256 |
| --- | --- |
| RE-SMAC-SCOUT-COMBAT-001, `scout-conventional-combat-cases.json` | `8a172801e23dfbaa76d189f5b079e7f2b02efe307801e5c2b1619cad8a91d401` |
| RE-SMAC-SCOUT-TAIL-001, `scout-attack-normal-return-cases.json` | `e968912e8515568d70bbd40118293f439501820d0863da20580020f83c1a22e0` |
| RE-SMAC-SCOUT-KEY-001, separate admission context only | `c2d7c06214d95d012377ea5e9012b20125b48667dea109551bc9a124339a04f3` |

The first record contains 187 original-x86 battles, 3,073 rounds and 6,194 CRT
draws. The second continues those battles through normal wrapper return and
adds three two-command sequences. Its original execution supplied 24 local
UI/engine destruction services; it did not replace gameplay or random routines.
This implementation compares those retained captures; it did not rerun the
original executable and does not claim a rendered or live-game observation.

`src/gse/tests/fixtures/SmacScoutCombat.h` owns normalized semantic data only.
HP is the recorded `10R - damage` value. Allowance is supplied as an explicit
fixture scalar resolved from the separate neutral Infantry movement handoff.
Original promotion weights are attacker/defender ordered; named winner/loser
expectations reorder them using the recorded winner. Original selected,
secondary, and continuation indices are translated through the actual
single-survivor fixture roster into distinct stable identities. The original
movement selector is retained as provenance, not offered as a combat stream
selector. No executable bytes, original assets, or research files are committed.

## Validation

Source parent: RNG `dd8234560d4f6463b24257a72737183fde4033c9`, based on main
`98dc39438621d62a3178067502953ca928dd7b87`. Six small native groups run inside the
existing full GSE test; no second full-suite/default CTest was added.

```sh
build/bin/GLSMAC --gse-tests --memorydebug \
  --gse-tests-native-filter '^SMAC Scout ' --prefix build/scout-combat-test-data
```

Local Ubuntu/Clang 15 Debug validation passed all eight existing CTests in
53.53 seconds, all six focused tracked groups in 1.08 seconds, and the same
groups with ASan/UBSan/LSan plus tracking in 1.07 seconds. Leak detection and
halt-on-error were enabled; the sanitizer executable used the existing local
non-PIE linker workaround without changing repository policy. One-TU LLVM 15
correctness analysis reported zero failures.

An optimized `-O3 -DNDEBUG -Wall -Wextra -Werror` build of the actual production
TU and RNG dependency was compared directly against the original JSON through
a temporary native bridge: all 187 complete transcripts/normal returns and all
three two-command histories matched. Eleven invalid-input Release controls
were rejected. This is retained-capture comparison, not new original execution.

The initial native run failed one group because the test treated participant-
ordered promotion weights as winner/loser ordered. The full corpus confirmed
that normalization error; only the fixture conversion was corrected. The failed
receipt is retained separately from the passing final run.

Seven copied-production controls, linked into the actual native executable,
test tie direction, round draw order, the elite outer promotion draw, uncapped
attack spending, source-square retention, selection versus continuation, and
zero-width draw suppression. Each exits 1 on the intended semantic mismatch;
the production source remains unchanged while controls run.

Local commands, generated fixture conversion, copied controls, Release bridge,
logs and results are retained under `/home/jhurliman/src/glsmac-scout-combat/`.
These are Linux source receipts; hosted CI and native Windows/macOS coverage
are separate. Future live integration needs an authoritative Unit representation,
context/command admission, actual owner/terrain inputs, and lifecycle ownership
of both RNG states. This component creates none of those defaults.
