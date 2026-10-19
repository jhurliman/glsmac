# Population-one owning capture: surviving defender

The original automatically **destroys an ordinary population-1 target** in this checked two-human context. The attacking Scout survives, enters the now-baseless tile, spends its full allowance, and movement returns success 1. No razing choice or captured-base screen appears. A matched return-to-former control instead retains population 1 and reaches the interactive captured-base screen. Exact contexts are in `population-one-capture-cases.json`.

Both controls use the established local two-human Librarian fixture, mutual Vendetta, equal technology, no objectives/projects, healthy full-move Scout 1, adjacent clear land, and a surviving defender HQ. Target Base 1 at `(20,16)` belongs to faction 2, has population 1 and facilities 3,4,7,8; the surviving HQ is Base 2 at `(26,16)`. Base 0 at `(16,16)` belongs to captor faction 1. Human mask is 6; the target's retained visibility byte is 2. The ordinary former/timer input is 2/0; the matched return-to-former input is 1/27.

| Effect | Ordinary capture | Return to former faction |
| --- | --- | --- |
| Target | Population decrements 1→0; base removed | Retained at population 1 |
| Owner/former at actual removal entry | **2 / 2**; owner has not transferred | No removal |
| Retained owner/former/timer | No target record | 1 / 1 / 0 |
| Base counts, captor/defender | 1/2→**1/1** | 1/2→**2/1** |
| Facility CRT draws | **0** | **0** |
| Facility setters | None | HQ clear requested, though absent; 3,4,7,8 survive |
| Production selection/reset | Skipped; target record disappears | Native target2, empty queue, minerals0, baseline -38/0, food5 |
| Notice | `SEIZEBASE` | `LIBERATEBASE` |
| Final boundary | Movement returns **1**, stack cleanup16 | Captured-base screen entry; no action supplied |

Both paths still take **66 credits**: `trunc(200*1/(2+1))`, using the stored defender population aggregate 2 before loss. Treasuries change `[100,200]→[166,134]`. The retained population totals stay `[1,2]`; they are not recomputed from surviving bases. Both seeds remain 3 and no CRT/shared RNG entry occurs. Both complete the normal 89-tech comparison with no technology unique to the defender. No restart is entered with the surviving HQ.

On ordinary razing, the code jumps past owner assignment, governor copying, random facility destruction, forced facility clears, captured-base production selection, and final captured-base recalculation. There is no population-zero captured base to publish. Its stored food, minerals, facilities, and queue disappear with the record. Loot has already posted before deletion.

Scout 1 moves `(19,15)→(20,16)`, spent movement0→3 of allowance3, damage remains0, and home remains Base0. The independent Scout0 is unchanged. The remote defender Former2 remains at `(30,16)` with the same owner/design/movement/damage. Native removal repairs its home to the surviving defender HQ and then compacts Base records: old Base2 becomes dense index1. Its saved home integer is therefore1 both before and after, but the referred base changes from `(20,16)` to `(26,16)`. Compare identity/coordinates, not just the unchanged integer.

The attacker's complete304-byte Base0 record is unchanged. The surviving defender HQ's complete304-byte record is copied unchanged from old index2 into index1. Canonical base count is2. The target's native post-command base lookup returns-1. Unit roster count stays3; selected unit1 and secondary-1 are preserved, as are design/plan counters. In the retained control there is no compaction and the remote Former's home becomes index2.

The ordinary target tile live items change `0x00101001→0x00100002`; Base marker1 is gone and Unit marker2 is present. Raw claimant byte7 becomes1. Complete44-byte before/after tile records, remembered words and cached affiliation are in the receipt. This claimant result follows the actual territory rebuild in this placement; it is not an unconditional destroyed-city ownership rule.

The native scratch current-base ID remains2 while its cache-valid key is-1, despite only dense IDs0/1 remaining. That scratch value is not a valid third saved base. Keep canonical roster/count and actual home links distinct from invalidated transient caches. The extended v3 receipt pins selected-unit references, all base records/counts, treasury, population, RNG, cached regional presence and retained faction-history fields. Observer-only v3 additions reproduce every v2 pre/post field exactly.

The sole ordinary notice is acknowledged at the previously verified display-only call site, whose return is ignored. Movement then returns normally, EAX1 and stack delta16. The retained control stops at captured-base screen entry `(Base1,0)` without executing screen construction or supplying an interaction. There is no newly supplied razing decision in either control.

Source gates outside this fixture can protect a base or bypass loss. This is not a universal population-1 destruction rule for all human/AI visibility, objectives or capture modes. Last-base elimination/restart is a separate control, not inferred from this surviving-defender result.


## Regional presence and retained history supplement

The inherited preparation has deliberately retained regional-presence bytes **3/3** for factions1/2 in region1, although live bases are1/2. The v3 snapshots preserve this discrepancy rather than silently recounting. Ordinary razing decrements only the removed owner's region1 byte, **3/3→3/2**. Return-to-former capture leaves regional bytes **3/3**, even though live ownership becomes2/1. Every other region byte is unchanged in both paths, and the28-byte region1 metadata record is unchanged. These are separate saved authorities from live owned-base totals.

The retained faction stamps at runtime offset0x14 (`95d1bc + faction*20bc`) start **2/2**. Razing leaves **2/2**; retaining/capturing the city sets the captor's stamp to current turn6, producing **6/2**. The two directional retained counter arrays at `95d334` and `95d354`, indexed by owner stride and counterpart*4, each change captor→defender **0→2** and preserve reverse0 in both cases. This is before-loss population1 doubled; the public receipt keeps the raw counters without inventing broader names. Elimination-history counters and supplied negative battleplan references remain unchanged with a surviving defender.

Use these literal cached values and deltas when comparing this fixture. A normal owner-summary refresh can rebuild regional presence later; the capture/removal action does not substitute such a refresh. v3 extends the frozen v2 evidence without changing its canonical outcomes.

## Port integration

The local owning Scout command now retains a size-one return-to-former Base and removes an ordinary size-one target when another defender Base survives. Razing prepares actual post-removal sight, territory, sites and home links, then publishes movement, bookkeeping, registry removal and frontend despawn together. The old Base and Pop wrappers retire even if a postcommit notification throws. No population-zero Base screen, production reset or facility lottery is synthesized. Stable GLSMAC IDs do not compact; their saved home references retain the identity of the surviving Base.

The live capture group covers ordinary capture, retained capture and both size-one branches, actual Start reload, validation with unchanged snapshots, existing late publication failures and postcommit removal-notification failure. The size-one razing cases pin native loot66, zero RNG, regional3/3 to3/2, unchanged stamps2/2 and paired counters+2. Last-Base size-one razing remains withheld until its distinct removal/disposition/ending continuation is established. General AI/objective/visibility capture modes remain outside the existing local ordinary Scout domain.

Validation: cached incremental build on the merged preview/map/capture integration; all eighteen live capture scenarios plus both last-Base capture consumers and riot transfer pass. Targeted correctness analysis passes BaseCapture, BaseAllocation and ConventionalMovement. Focused ASan/UBSan instrumentation of six production and two fixture translation units passes the extended live capture group and both last-Base consumers, including postcommit retirement on notification failure. Remaining linked objects use the normal cached build; leak detection is disabled. This is not a full platform or whole-engine sanitizer claim.
