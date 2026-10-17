# Selected Spartan Rover movement at one friendly HQ

This extends [friendly HQ movement](conventional-friendly-hq-movement.md) to the
actual owned, selected Spartan startup Rover. It does not activate default
startup or admit arbitrary Speeder designs. Existing base-free movement is
unchanged.

## Admission and ownership

The existing bound simulation resolver must return `SelectedSpartanRover`.
That resolver first validates the canonical participant, Spartan source profile,
owned design and exact saved `starting_scout` selection. Movement neither derives
a design ID from a transport slot nor substitutes the predefined Unity Rover.
The native fixture obtains the selected ID through the actual copied GSE query.

The additional measured HQ context is healthy stored morale1, zero damage,
Speeder/Gun/Scout/reactor1/no abilities, frozen Q3, basic/full allowance6,
retained MORALE2 and PLANET0. The retained ratings follow the checked startup
refresh; they are not unit-creation defaults. The earlier Infantry context
remains separate and unchanged.

All prior constraints remain: local single-player with one admitted controller,
explicit saved no-project campaign capability, one registered Unit and Base,
canonical same-owner/faction population1 HQ with exactly its canonical
HEADQUARTERS facility, HQ at one endpoint, empty adjacent other endpoint, no
animation/locks/hostile neighbors, and the existing flat ordinary/road/tube/fungus
endpoint profile. Home remains explicitly None. These global cardinality and
rating restrictions bound this receipt; they are not claims about universal
original movement eligibility. Remote HQs, additional units, distant fungus,
wounded units and wider faction contexts remain separate work.

The existing original-backed edge and lottery kernels still calculate the
result. The authority preparation, allocation, Unit/tile/RNG commit and
postcommit frontend publication are byte-for-byte unchanged. Exhaustion rejects
before edge pricing even at a tube endpoint. No wire version changes: existing
selected design, Unit, HQ and paired RNG values already persist this state.

## Original comparison

The human-owned RE-SMAC-SPARTAN-HQ-MOVE-001 executes 21 complete original
mode3 requests, using transferred actual startup-produced Spartan Unit/design,
faction records, refreshed ratings and first HQ. Original placement and lookup
routines establish real membership. The wrapper/body, allowance, cost, lottery
and relocation execute to normal return. The transferred fixture is not one
continuous complete new game. Neutral keyboard state, allocator and final UI
cleanup remain supplied; no display, hostile base, stack or repair claim follows.

The committed 21-row projection retains every ordered input ID and checked
`actual` outcome from the private report. It compares the actual port move event's
spent byte, failure marker, shared RNG word and source/destination membership,
with unchanged CRT/JSF, identity, terrain and complete Base serialization.
It covers flat Q3, road1 and tube0 at either HQ endpoint; partial and exhausted
allowance; HQ-to-fungus9 strict draws5/6 and1/2; and fungus-to-HQ partial draws.
The exhausted cases produce no Unit update; admitted lottery refusals do.

Pinned evidence (read-only; original assets/reports are not committed):

- Executable: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- Public `docs/behavior/spartan-hq-movement-cases.json`:
  `3b2991051b1d4f724f4403f65a4e9ed9c0b4b4f0a74036d20d2dded39f183669`.
- `tmp/reference/check_spartan_hq_movement.py`:
  `cbac70072a02138ea183fafee6d1536d498a782559cbd81ed278daad809e3557`.
- `tmp/reference/spartan-hq-movement-original-x86-results-20261017.json`:
  `60156db4944b3793c25c9d6d4b627cea54a552756566e2f2b1d930d78f0faa24`.
- Produced Unit/design byte hashes in that report:
  `475f478790c054a1ce979de85f463eb783f7b79cfb4c7fb1d978e928bd51eb2e` /
  `4cc10f682f2b446272cca3257e630b067c8f5e83df5ffbda7ffc584cf4c7fc67`.

The separate startup morale contract establishes Spartan retained MORALE2 after
refresh. The report also pins nine dependency sources; no original oracle code
is imported into production or native tests.

## Validation

Source parent: `4e5ddffdddd6daf5f64b23f549f6473dccb40547`, including the separately
reviewed selected-presentation prerequisite and Infantry HQ extension.

Three new native groups cover all21 original outcomes, eight rejected contexts
without authority/presentation effects, and actual world-body restore followed
by real upkeep and the next move. Rejections distinguish damage, stored morale,
PLANET, retained MORALE, Q, no-longer-selected designs, a copied alternate selected
design, and an extra remote Unit. Existing HQ metadata/terrain/identity and
selected-presentation tests remain registered. The fixture's single-player
Spartan is accepted before new-world production; its HQ uses that canonical
owner's faction. No live socket, rendered scene or default-world claim is made.

The prior production fails both positive new groups (first original move and
saved failed attempt); its rejection group passes. The early fixture-only
private-query compile error and hardcoded copy-source failure were corrected
before recording that baseline. Intended production was not changed for it.

- Full normal CTest: 8/8, 94.68 seconds.
- Full leak-enabled ASan/UBSan/LSan CTest: 8/8, 164.09 seconds.
- The same eight focused selected-Rover/HQ groups pass with allocation tracking
  in both builds, including the two existing selected-presentation groups and
  three existing Infantry HQ groups.
- Explicit LLVM15 analysis of the sole changed production translation unit:
  zero failures, 2.55 seconds. No production header changed.
- Two copied production controls fail at the intended assertions: disabling the
  selected-Rover path fails original case01; dropping its retained-MORALE gate
  admits rejected context3. They never modify the intended checkout or binary.

Validation logs, copied controls and a source/log manifest are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/` with `rover-hq` names. This is
source validation; no new interactive or platform-wide receipt is claimed.
