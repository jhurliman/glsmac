# Conventional ordinary ground movement with remote Bases

The local conventional movement event now admits the measured nonfungal edge
when the world has remote Bases and neither endpoint is a Base. Previously any
Base-containing world required an endpoint HQ. This is a narrow admission
extension to the existing stat, edge-cost, spending and atomic commit paths;
default startup, UI, saved formats and the movement kernels are unchanged.

## Admitted boundary

The Base-containing route still requires a Ready healthy Unit with stored
morale 1, damage 0, no home, frozen Q3 and either Infantry design 2/basic 3 or
the actual selected Spartan Rover/basic 6. Current retained ratings and the
existing checked full allowance remain authoritative; no faction-name or
neutral-rating gate is added. The literal cases use the actual Gaian and
selected Spartan profile producers.

For this new route, both endpoints must be flat, feature-free land. Their
terraforming fields must be exactly the same: neither field set, road only, or
tube only. Mixed infrastructure, combinations, fungus, river, forest and rocky
terrain remain outside this new admission. Existing endpoint-HQ handling and
the destination-fungus fence remain unchanged. The older base-free route keeps
its existing scope; this extension does not reinterpret its evidence.

The canonical BaseManager's centers must lie outside both endpoint
neighborhoods. This preserves rejection of a broken endpoint Tile-to-Base link
and keeps adjacent Bases out of the remote-only extension. The neighborhood
restriction is a bounded port policy, not a universal original-game rule.
Remote Base contents do not supply movement modifiers or a nearest-base value.

Existing running/local single-player controller, canonical owner/design/tile,
saved NoProjectsOrdinaryTerrain capability, exact unstacked source, empty
destination, locks, animation and nearby foreign-Unit checks are retained.
Preparation still computes with copied RNG. Destination/update allocations
precede the unchanged nonthrowing Unit/tile/paired-RNG commit; unsupported
requests cannot partially consume a draw or move membership.

## Evidence and real event tests

RE-SMAC-REMOTE-ORDINARY-GROUND-001 contains 12 remote requests and 12 matched
one-unit/no-Base controls. The original executes the complete normal movement
wrapper/body using startup-produced Gaian Infantry and Spartan Rover records,
on a controlled 128x64 world. Remote requests have the own HQ at (40,16), six
foreign HQs and Scouts at (52..112,40), and a local edge (16,16) to (17,17).
Both original endpoint Base lookups return none. The paired body-call sequences
and movement outputs match; all remote records and the CRT word are unchanged.

The committed owned fixture projects all 24 ordered literal results:

- Full flat movement costs 3.
- With one point left, flat movement draws once from the shared stream; success
  spends the full cost, and refusal exhausts allowance and sets the marker.
- Connected road costs 1; connected tube costs 0.
- An exhausted Unit refuses even a zero-cost tube before edge pricing.

The source fixture SHA256 is
`98d98b69c7ffb3a585a9f2a44b48242d755fcf9b7ca59d9a3a38721d5a1424b9`;
the original report SHA256 is
`08ccad0e0b7774bcf5af2db6c383da695563ea543cf3b7fe96c4e5a53b1f194a`.
The report binds executable
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`
and original probe source
`5cf43878f8645fe46bcdb835719f136bbb1fc0074260a0e9c042da5f07e0824f`.
Projection verified the complete ordered actual-result objects against their
retained expected rows before emitting the small header. No original executable
or probe code is rerun or imported, and no proprietary asset bytes are added.

Four native groups use the actual GSE move event, canonical participant/design
binding and real Unit/Base managers. Port IDs are explicit fixture identities,
not original numeric IDs. The 12 remote cases preserve every Base and remote
Unit byte, remote tile membership, both gameplay RNG words, JSF and next Unit ID.
The 12 empty controls contain no Bases and one live Unit; the fixture retains
its accepted participant catalog rather than claiming an original reset/deletion
procedure. Remote port participants use valid frozen profiles; their unrelated
ratings are not a reproduction of the original supplied neutral remote state.

Twenty-six unsupported contexts check rejection before authority changes,
including both-sided terrain restrictions, mismatched/combined infrastructure,
wounds/morale, stacks/occupants/foreign neighbors, wrong caller, unavailable
capability, network role, locks, animation, water and adjacent Bases. Existing
HQ tests retain malformed endpoint-link coverage. Four old missing-HQ-only
negative cases now explicitly use forest or fungus, because their former plain
edge is the behavior intentionally admitted here.

Both profile restore tests save an actual failed partial move, restore through
the real world-body decoder, compare complete canonical state, run actual next
turn upkeep and execute the next event. The existing comparison sorts only the
two complete named UnitManager definition tables; Unit records and every other
world frame remain exact. This does not claim unordered-map wire order, a full
rendered initialization, network command support, repair, combat, ZOC, distant
fungus/native-life encounters or arbitrary generated-world movement.

## Validation

Source parent is `2f3b707bc431310d34c82912dda2ab5bcea5b4cf`. Before the production
change, the four new groups yielded two passes and two failures: empty controls
and unsupported rejection passed; remote row 01 and the failed-move restore
setup failed. Final full normal CTest passes 8/8 in 97.13 seconds. Full
ASan/UBSan/LSan CTest passes 8/8 in 218.92 seconds with leak detection and
halt-on-error enabled. All 19 affected groups pass with allocation tracking in
both builds. LLVM 15 analysis of the one changed production translation unit
reports zero failures in 2.88 seconds.

Four copied-production controls fail the intended unsupported-context
assertions: omitting the feature fence, matching-infrastructure check, canonical
Base-center exclusion, and healthy-damage condition. The intended source and
binaries are unchanged by these controls. Literal fixture regeneration is
byte-identical. Initial full runs exposed the historical missing-endpoint-only
negative cases described above; those logs are retained separately. No Windows,
macOS, hosted CI, original-execution rerun or rendered-gameplay pass is implied.

Local logs and the pinned projection driver are retained under
`/home/jhurliman/src/glsmac-remote-ordinary-ground/` and the worktree's ignored
`out/` directory. Human-owned research files remain untouched.
