# Canonical tile resources and Base intake

Opted-in SMAC Bases now use the existing center and ordinary-tile resource
calculators through actual `Tile::GetResources` and the default
`Base::GetIntake` callback. The callback still sums the center and the saved
worker assignments. Queries return fresh N/M/E values and do not allocate
workers, refresh social state, age events, draw RNG, or post stored output.

This slice depends on the owned resource, technology and faction profiles,
saved global modifiers, canonical Base context, Facility source roles, and
the saved [DisplayedTerrainV1 map policy](glsmac-map-resource-policy.md).
It introduces no new snapshot format. Legacy Bases keep their existing
callbacks. Ordinary player-only tile previews remain on that legacy route;
the canonical path requires its evaluating Base.

## Actual source inputs

`Game::GetCanonicalTileResources` validates the current bound simulation,
registered Base and canonical Tile before producing calculator inputs.
It requires the saved `NoProjectsOrdinaryTerrain` supported-campaign
capability. This is the controlled GLSMAC campaign restriction; it is not an
inference of original project ownership from empty facility sets. A future
project consumer must retire that capability before activating projects.

The adapter uses the evaluating owner's **current** ECONOMY and PLANET,
not retained movement/combat ratings. Social state must already be Refreshed.
Fungal N/M/E is derived through the existing owned-technology/faction-bonus
producer on each query. Configured limit/platform technologies retain their
frozen positional identities. Ordinary availability keeps None, Disable,
Unknown, owned definitions, the asymmetric None+Disable prerequisite case,
and unavailable position 88 distinct. It does not recursively require parent
technology ownership. An owned definition with an unresolved prerequisite
rejects rather than silently becoming disabled.

Facility pointers must match current manager definitions before their roles
are read. The adapter uses source positions 1 (HQ), 3 (Recycling), 13 (Tree
Farm), 14 (Hybrid Forest), and 26 (Pressure Dome), independent of definition
IDs or display names. Other recognized local facility positions 1..33 have
no extra effect in these calculators. Unbound roles, orbital positions,
Stockpile and projects reject. All registered Bases must have valid canonical
contexts and admitted facilities; their actual owner identities determine
the evaluating faction's Base count. This deliberately does not infer neutral
state for remote legacy Bases.

Golden Age and the six local resource-event flags come from the owned Base
context. The timer remains unchanged by queries. Dust is active for any
nonzero saved countdown, including negative values. Planetary Governor uses
the optional saved simulation identity, independently of Base governor
options. The native query takes an explicit Ordinary/Upkeep phase and selects
the corresponding saved solar flag. The actual display/intake callback uses
Ordinary; no execution timing implicitly selects Upkeep.

## Map and branch boundaries

Actual Tile moisture/rockiness enums are validated before translating their
1-based port values to calculator values. Altitude and visible resource bonus
come from DisplayedTerrainV1, including its five-sample water classification
and strict displayed depth boundaries. These are named GLSMAC semantics,
not original generator or height equivalence.

River, Monolith, fungus and Jungle flags are admitted; land Jungle supplies
the measured nutrient landmark input. Other landmark/region/detail effects
are unavailable and reject. The admitted improvements are road, tube,
forest, farm, soil enrichment, solar collector, mine, condenser, borehole,
sensor, bunker and airbase. Unknown bits reject. Echelon Mirrors reject,
including a neighbor of an evaluated collector, because owner affiliation
is not represented by the current Tile. This is a conservative admission
limit; no mirror count or owner is manufactured.

The evaluating Base's own center uses the center calculator. Any other Base
center rejects before ordinary-tile dispatch, including when an existing
worker assignment points there. The adapter does not silently repair that
assignment. Center arithmetic retains the existing measured signed32 wrap;
ordinary tile arithmetic retains the existing checked signed32 domain. N/M/E
results and actual script accumulation use signed64 values. Returned
technology-cap excess diagnostics are not installed as an authoritative cache.

`Base.get_smac_nutrient_demand()` separately returns the frozen
nutrients-per-citizen value times actual population with a checked signed64
magnitude bound. It accepts signed configured rates; it does not hardcode 2.
**Canonical `get_consumption()` rejects** because unit support and facility
maintenance are not yet complete consumers. The new scalar is not a full
consumption triple and no mineral/energy zero balances are invented.

## Exercised paths

Eleven dedicated native groups use the existing asset-free Backend fixture,
actual host profile freezing, canonical participant admission, actual Base
and Facility managers, and the real default `get_base_intake` callback. No new
runtime constructor or alternate callback implementation supplies the canonical
yield. The fixture explicitly installs the already-tested map policy on its
synthetic map; actual generation/policy publication remains covered by the
separate map-policy checkpoint.

The seven faction cases project the literal center/forest/fungal/intake
outcomes in RE-SMAC-FIRST-WORKER-CONTEXT-001 onto independently owned source
metadata and an explicitly supplied existing worker selection. Public fixture
SHA256 is `84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab`.
These checks compose the live port query path; they do not rerun the original
allocator, full setup, manual worker controller, or forced general calculation.

Additional tests cover modified frozen rows and demand, current-versus-retained
ratings, independent solar phases, negative dust, council role, Golden Age,
local events, Base count, positional facilities, fifteen actual terrain cases,
configured technology ownership/sign boundaries, unsupported source inputs,
wrong owner, coherent Early/turn-zero rejection, other centers, center wrapping,
ordinary overflow, wide actual
intake totals, detached results, and unchanged Base/RNG/presentation state.
Legacy callback and consumption routes have a positive control.

The actual world-body restore uses saved map policy, global/social/technology
state, typed facility definitions, Base context and selected worker coordinates.
It starts without a current simulation and confirms no founding/reset replay.
The complete world comparison sorts only the ResourceManager and BaseManager
unordered tables, retaining each full record and all other bytes. It does not
claim raw cross-instance serialization order. Exact restored bytes are checked
again before/after queries, and the real intake result must remain identical.

## Remaining live consumer work

This enables canonical gross tile intake and explicitly named nutrient demand.
It does not complete support, facility maintenance, commerce, inefficiency,
specialist E/P/L allocation, citizen effects, event aging, stored-resource
posting, or full manual-worker recalculation. Existing canonical worker/growth
guards remain. A controlled playable scenario still needs an initial population
package and the remaining full balance/UI contracts; this change does not
activate the default campaign or make base-screen consumption silently valid.

## Validation

On source parent `4e141136` (map policy plus canonical Base context and Facility
identity), the full normal CTest suite passed 8/8 in 103.55s. After the final
test-only ownership correction and coherent Early-state assertion, all eleven
groups passed with native allocation tracking in 4.033s normally and 5.096s
under ASan/UBSan/LSan. The final leak-enabled sanitizer CTest suite passed 8/8
in 215.52s. Full LLVM 15 correctness analysis covered 342 translation units
with zero failures in 254.32s; the final changes after that scan were confined
to the test fixture and this receipt.

Six private copied-production controls each failed the intended actual-path
assertion: omit the other-center fence, substitute retained ECONOMY, identify
HQ by its port name, bypass configured technology availability, hardcode the
nutrient rate to two, and omit the Refreshed phase fence. No checkout source or
normal build object was mutated for those controls.

The earlier sanitizer failure is retained: the missing-resource fixture used
`UndefineResource`, which detaches ownership, then allocated a replacement.
The fixture now owns the detached definition until it restores the same
object. Production code did not change in that correction. Normal full-suite
timing precedes these last fixture assertions; the final focused normal and
full sanitizer runs include them.

Logs, copied-source controls, full analysis results and their hashes are retained
outside Git under `/home/jhurliman/src/glsmac-unit-weapon-parser/`, with final
manifest `intake-final-manifest.json`. These are source-checkpoint receipts;
they do not claim a rendered campaign, socket transfer, or later integration
validation. No proprietary asset or original binary is committed.
