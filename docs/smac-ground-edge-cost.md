# Scoped ordinary ground edge costs

`SmacGroundEdgeCost` implements a pure subset of
`RE-SMAC-MOVE-COST-001` (`docs/behavior/movement-edge-cost.md`). It supplies an
integer cost for an already resolved ordinary ground query. It creates no
runtime Unit, changes no map or RNG, and reads no installed file.

## Input contract

`smac_edge::Query` owns its design metadata, source and destination values and
query context. `QueryGroundEdgeCost` returns an owned signed32 integer.
Unsupported inputs throw `std::invalid_argument` before any early return.

The admitted profile is deliberately narrower than the original query:

- Explicit original positional Infantry0 or Speeder1, ground domain0,
  projectile weapon mode0 with positive offense, plan0..3, no abilities.
  Negative IDs and native special IDs8/9 are rejected. This is not workshop
  eligibility, a catalog resolver, a component-name heuristic or a unit factory.
- Positive simulation faction ID1..7. This is not a transport slot index.
- Q1..100 and a supplied positive basic design allowance of Q..99Q, exactly a
  multiple of Q. These are port admission bounds. The original retained edge
  corpus exercises Q2/3 and installed Infantry/Speeder speed1/2. Basic allowance
  is distinct from a unit's full, damaged or remaining movement. The separate
  movement-rule producer can supply it; this query does not duplicate its speed
  byte/clamp calculation or infer Q.
- Explicit signed32 **retained PLANET** and an explicit resolved
  `owns_xenoempathy` boolean. Current PLANET is intentionally not an input.
  Absent and other-faction ownership both map to false in the retained fixtures;
  the query does not resolve project ownership itself.
- Explicit `suppress_terrain_charges`, corresponding to the original query's
  nonzero terrain switch. It suppresses only rocky/forest additions. A caller
  must select it from its own proven contract, not infer it universally from
  speed. The original ordinary movement caller supplies zero.
- Land altitude bands3..7 and rockiness0..2, with only these original feature
  bits: base0x1, road0x4, tube0x8, fungus0x20, river0x80, forest0x200000.
  Unknown bits are rejected; a future map adapter must explicitly establish
  which features are relevant/supported rather than silently masking them.
- Nonnegative in-bounds doubled coordinates with matching X/Y parity, positive
  signed32 height and even signed32 width at least2; horizontal wrapping is
  explicit. This arithmetic domain does not admit dimensions for map creation.
  There is no Y wrapping.

Sea, air, Hovertank, native faction/design, Psi, special plans and abilities are
outside this profile even where a diagnostic original query has the same
numeric result. There is no SMAX claim. A caller cannot use an infrastructure
early return to bypass the unsupported-input checks.

## Ordered calculation

For the admitted land-to-land profile, the original early returns are preserved:
connected base/tube endpoints cost0; connected base/road endpoints cost1, with
owning-Xeno source fungus also supplying that source endpoint; owning-Xeno
destination fungus costs1; then diagonal river connection costs1. River geometry
requires wrapped absolute X distance1 and absolute Y distance1. A doubled-X
horizontal step(2,0) does not receive the discount. A flat-map nonadjacent query
is still a query, not permission to move.

Otherwise cost starts at Q. Rocky destination and forest each add Q unless the
terrain switch suppresses them. Destination fungus then uses
`max(terrain_cost, basic_allowance)` for positive retained PLANET, or adds2Q
otherwise. Coordinates and arithmetic are evaluated in signed64. The input
bounds limit the result to `max(5Q,99Q)`, at most9900, before narrowing.

The output is not command legality. Adjacency, enemy contact, occupancy, ZOC,
treaties, boarding, fuel, keyboard admission and actual relocation are absent.
The separate empty-ground movement model still enforces its own narrower
nonwrapping byte-spend domain. An edge cost above its domain must not be silently
clamped, and a cost above full allowance is not by itself a rejection rule.

## Original evidence retained

The read-only original audit reports2,731 complete original cost-query executions:
43 named,2,016 terrain-grid and672 infrastructure-grid results. The checked-in
native fixture retains **768** results: **24 named +576 terrain +168
infrastructure**. It excludes1,963 queries for unsupported design, native-faction
or sea profiles. No excluded profile is relabeled as supported.

Inputs for the two grids were reconstructed in the exact ordered loops from the
pinned original harness; expected costs are its retained **actual original EAX
results**, not recomputed by the new production function. All IDs/order and the
report's success counts were checked during fixture generation. Named rows were
also obtained from the original JSON. The reduction supplies installed ordinary
basic allowance Q or2Q and omits independent current PLANET because the query
does not consume it; both contrasting current/snapshot named controls remain.
Synthetic tests separately exercise competing tube/road/river/fungus returns,
X-only seam behavior, wide arithmetic, ownership and unsupported-domain rejection.

Pinned inputs (SHA256):

| Input | SHA256 |
| --- | --- |
| Original executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Installed alpha | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
| `movement-edge-cost-cases.json` | `2062a06618859b304e688cd81f207dace6387af22ff00cb079651c7a4360db51` |
| Original result report | `3186dc810755267d564dba2c15584dccfe54d677bd3c9cb6cf12c72551b99f0b` |
| Private `check_movement_edge_cost.py` | `6622405977faebc0b10644912854e5f178aceeb8d5630946f9666a75fa9a76f4` |

The original executable, installed assets and private report are not committed.
This implementation compares against retained captures; it does not claim a new
original-executable run, a full-game comparison or live movement activation.

## Validation

On the main-based source branch, the normal native suite passes all8 CTests
(58.56s), including five new focused groups. Focused allocation tracking passes
all five groups (1.05s), and one-production-TU LLVM15 correctness analysis reports
zero failures. Recompiling the production TU with `-O3 -DNDEBUG` and linking it
into the same native harness passes all five groups, including all768 original
comparisons; this is a Release production-TU check, not a second full Release
application build.

Seven copied production controls fail their intended groups: omit tube return,
admit destination-only road, discount horizontal river, omit cheap Xeno fungus,
replace terrain with basic allowance, suppress fungus with the terrain switch,
and admit native IDs. Each runs the real native tests; intended source remains
unchanged. No failure depends on allocator reuse or an unsafe operation.

The same five focused groups pass ASan/UBSan/LSan with allocation tracking
(1.09s), `detect_leaks=1:halt_on_error=1` and UBSan halt/stack traces. The local
Linux sanitizer linker uses the previously documented `-no-pie` workaround;
there are no production suppressions or sanitizer changes. A second unchanged
full sanitizer suite was not run for this pure query.

Local command (Debug native-filter support required):

```sh
GLSMAC --gse-tests --gse-tests-native-filter '^SMAC ground edge cost ' --memorydebug
```

The focused checks require no original assets. This slice has no Unit, Game,
Settings, world-format, script or live renderer changes.

The manufactured-unit integration additionally admits Infantry Terraformer
mode9/offense0/plan9 and, on nonfungus endpoints, Infantry Colonist
mode8/offense0/plan8. The latter reuses the same ordinary terrain arithmetic;
its actual forest/flat caller continuation is checked in the Colony production
tests. This does not admit noncombat fungus encounters or foreign ZOC.
