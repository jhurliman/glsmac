# Conventional HQ movement with nonneutral ratings and remote records

The healthy friendly-HQ route now composes the existing unit-stat, edge-cost
and movement-admission kernels with actual retained ratings and canonical
endpoint identity. It no longer requires neutral MORALE/PLANET, a single
registered Unit/Base, or a single accepted simulation participant. This extends
[Infantry HQ](conventional-friendly-hq-movement.md) and
[selected Spartan Rover HQ](conventional-selected-rover-hq-movement.md) admission;
it does not activate default startup, AI commands, network movement or combat.

## Authority and remaining domain

The GLSMAC command policy remains local single-player: no Connection, local
network type/role None, running game, and an accepted current local participant
with the Single controller role owning the moving Unit. Other canonical
participants may coexist. Their simulation IDs come from the saved binding;
transport indices, names and faction templates never rebind an owner. This is a
port caller policy, not a claim about original multiplayer/human/AI admission.
The original LOCAL-ROSTER fragment separately confirms that local single-player
transport does not imply a one-faction active set; its supplied active/human
sets do not establish a complete seven-faction startup or turn.

The Unit must be Ready, healthy with stored morale1/damage0/home None, and either
the existing Infantry design2 or the actual owned selected Spartan Rover context.
Frozen Q3 and basic allowance3/6 remain required. Fresh queried effective morale
and full allowance, and the owner's retained PLANET, feed the existing kernels;
there is no added faction-name or neutral-rating eligibility test. An Early raw
Unit still rejects before reading an absent retained context.

A Base-containing world requires a registered HQ at a movement endpoint. The
borrowed endpoint pointer is first located in the canonical BaseManager before
its fields are read. It must have the same canonical owner/faction, population1
and exactly the canonical HEADQUARTERS facility. The other endpoint is adjacent,
flat and empty, with the existing none/road/tube/fungus restrictions; HQ terrain,
feature combinations, wounded repair, hostile bases and non-HQ endpoints remain
unsupported. Remote bases may have another owner or no HQ. Neither endpoint
being a HQ remains outside this route, including distant fungus.

Source membership is explicitly one exact registered Unit; destination occupancy,
locks, animation and foreign Units at either endpoint or their neighbors reject.
The saved NoProjectsOrdinaryTerrain capability remains mandatory. Remote objects
do not weaken these checks or create project eligibility. An adjacent endpoint
HQ establishes nearest-any-base distance at most1. Another base winning an
equal-distance tie changes its ID, not this consumed distance condition; the
implementation neither requires nearest ID to equal HQ nor invents a distance
producer for the unimplemented farther branch.

Only preparation/admission changes. Allocations still precede the synchronous
nonthrowing Unit/tile/paired-RNG commit, and presentation remains postcommit.
No new wire version, entropy, JSF draw, animation callback, home inference or
rollback ownership is introduced. The separate
[destination-fungus correction](conventional-fungus-admission.md) now rejects
that path without an admitted endpoint HQ. The earlier base-free original
complete-request fixture inherited a zero nearest-distance value when no Base
existed; it does not establish arbitrary initialized no-base native-life/CRT
behavior.

## Literal original comparison

Three human-owned complete-request receipts ground this admission delta:

- GAIAN-HQ-MOVE:8 requests use transferred actual Gaian Scout/HQ state,
  stored1, retained MORALE-1/effective0 and PLANET+1. They exercise flat partial
  pass/refusal at both HQ endpoints, fungus cost3 and strict partial draws,
  and exhausted tube refusal. Static argument provenance permits composition
  of the already measured rating/allowance kernels, without a Gaia-only gate.
- REMOTE-HQ-MOVE:9 requests add six foreign remote HQs/created Scouts to the
  selected Spartan Rover, preserving complete body call vectors and outcomes.
- REMOTE-HQ-ORDER:12 requests add a same-owner non-HQ base/Scout, reorder the
  chosen Unit/HQ records, and include four equal-distance non-HQ/HQ ties. Actual
  selected lookup IDs change while local outcomes and all remote bytes remain
  unchanged. No additional global count-one condition appears in those routes.

The committed headers project all29 ordered inputs and literal `actual` results,
without importing original probe/oracle code. The actual port move event compares
spent byte, failure marker, paired RNG and local membership. All Base records and
remote Unit records/membership remain unchanged, and JSF is unchanged. The port
fixture uses accepted canonical participants, actual frozen-profile refresh and
real definition/spawn/event paths on a synthetic128x64 map. Remote port profiles
are accepted fixture profiles, not a claim of the original neutral remote
producer. Port positive Unit/Base IDs model record order; they are not original
zero-based array IDs. No original executable or graphical game was rerun here.

Pinned SHA256s (original assets/reports remain uncommitted):

| Evidence | Public fixture | Private actual report |
|---|---|---|
| Gaian8 | `1026ea4f056acb9597c48ac754fefc09373603eb929c48b04b5cbb00c04f3a93` | `128c064b7becb7ed4ef1600888f14042b9d1999f04c8aec790e5a62e3518532a` |
| Remote9 | `550b61db461d78df2ff4435b1170b30d14bfdd245454fff03a5a258ebc61def4` | `c2718c3721fbc696179f8308218507f8370a2b7e9f0315344539adb37a24f999` |
| Order12 | `baecd2226d82ded06447b79b706d31d58a2001c7dc750cabe28fa81220cdf192` | `89c9976ea0971b4a181bd4a993e40cf45e95289f6560102e830cfefbc23aa346` |

Fixtures are `docs/behavior/{gaian-hq-movement,remote-hq-cardinality,remote-hq-order}-cases.json`;
reports are `tmp/reference/<same-name>-original-x86-results-20261017.json`.
All bind executable `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The original checker hashes are respectively
`ddaf1aced88bf48b9ede157e5d141b562a9272f5b47c4f7b759856f64be3b0d6`,
`2e9e8d4e3f085ffa8ea444c7339394b991b4f6c21119a5854c43191033549094`, and
`b3142543e7bbf54d7e947c28eec3ca6fdcc5e654c799110b3bc83a57a4c67851`.
Nearest-any-base14 and local-roster14 are supporting source contracts, not
additional port differential cases.

## Validation boundary

Source parent: `b2deeacae90494e61418f7f8ff082526a9730c77`, including the Early
lifecycle/Ready guard and the prior selected Rover HQ change.
Four new native groups cover29 literal requests, nine rejected local-controller/
interaction contexts, and both Gaia/Rover actual world-body restoration followed
by real upkeep and the next command. Prior unsupported-rating/global-cardinality
rejections are replaced with supported-capability, source-lock/stack and two-HQ
endpoint rejections; other earlier tests remain registered.

The restore test compares the complete saved world semantically. Existing
UnitManager definition tables use unordered-map iteration, so two coexisting
Scout/Rover definitions can serialize in another order after restore. The test
sorts only those two named definition tables, compares every complete name/value,
and retains exact bytes for the whole remaining header/simulation/map/resources,
Unit records/next ID, Bases and animations. The subsequent real command produces
identical complete canonical state and paired-RNG continuation. It does not claim
byte-identical unordered wire order, socket transport or rendered startup.

- Full normal CTest:8/8,96.63 seconds.
- Full leak-enabled ASan/UBSan/LSan CTest:8/8,185.65 seconds.
- All10 focused HQ/controller groups pass with allocation tracking in both builds.
- Explicit LLVM15 analysis of the sole changed production TU:zero failures,3.04
  seconds. No production header changed.
- Four copied production controls fail the intended assertions: restore the
  neutral rating gate, restore global count-one, choose the first Base instead
  of the endpoint, or omit the source-stack guard. The intended checkout and
  binaries are never changed for these controls.

The old production failed actual Gaian case01 before the admission change.
Initial fixture compilation fixes and its previously hardcoded8x8 presentation
MapState were corrected before final receipts. The expanded fixture now derives
presentation dimensions from its actual accepted128x64 synthetic map. An initial
whole-wire comparison exposed the unordered definition order described above;
all final comparisons include complete canonical state rather than hiding that
ordering difference. Private logs, copied controls and the final manifest remain
under `/home/jhurliman/src/glsmac-unit-weapon-parser/` with `hq-composition` names.
