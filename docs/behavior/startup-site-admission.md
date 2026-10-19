# Turn-zero site search and final starting position

The [runtime reference](startup_site_admission_reference.py) reproduces the original seven-faction Planet search from terrain, region metadata, current cache, existing Bases/Units and current CRT state. It matches **3,747 generated candidates, 7,494 random draws, every rejection branch, 65 scored refinements, three uncached queries/cache writes and all seven resulting maps**. [Cases](startup-site-admission-cases.json) contain numeric evidence and ordered-candidate hashes. No candidate coordinates, ratings or random outputs are supplied to the reference.

This closes selection upstream of the [site-balancing deficit](startup-site-balance.md). The quality callback uses the existing [land](site-quality.md), [water](sea-site-quality.md) and [turn-zero](site-quality-late-turn-and-startup.md) rules; the balancing heuristic is a different calculation.

## Preparation and cache authority

After climate/height/region preparation, native terrain postpass clears every rating while preserving the low affiliation nibble. It also regenerates each region's count of wet, nonfungal land tiles with rockiness below2. In this control the following territory/cache pass queries and stores all3,200 tiles in row order. Independent quality formulas match every stored value and all128 regional wet counts. Those resulting ratings exactly match the first faction's search input; the imported file's old ratings are not the authority after this preparation.

The refill is conditional in its world-preparation caller; this profile takes it. Preserve the existing [territory/cache lifecycle](territory-cache-refill-and-radius-repair.md) after Base creation: exclusion can bypass a nonzero rating without erasing it, nonzero ratings are reused, and stored zero remains a cache miss. Later startup refill calls repeatedly query zero-rated tiles. Do not clear or recompute every rating at each search candidate.

## Candidate generation and measurements

Let n start at0. Compute margins `mx=clamp(trunc((1024-n)*width/8192),0,9999)` and similarly my from height. On a wrapped map, choose x from the whole width. On a nonwrapped map, choose x from span `width-2*mx`, then add mx. Choose y from span `height-2*my`, then add my. A span above1 consumes one CRT draw and uses its result modulo span; smaller spans use0 without drawing. Clear x's low bit, then add y's low bit to enforce parity. Increment n before admission checks.

Every stock candidate here consumes exactly two CRT draws. The controller supplies the current seed after faction initialization; do not restart every faction's search from seed1 or from the fixture's recorded per-faction seeds.

Before checking admission, calculate:

- Good tiles: count valid inner21 tiles with elevation band>=3, positive moisture, rockiness<=1 and no fungus.
- Landmark bucket: first ring0..7 containing a tile whose landmark word has a positive signed high byte; otherwise32. Use original ordered rings, wrapping/clipping each coordinate.
- Nearest Base and distance in the candidate's region, with later Base IDs winning ties. If no Base qualifies, set distance32. Reduce it using same-region existing Units.
- Region presence mask: retained nonzero faction/region presence bytes OR same-region Unit owner bits. Do not substitute a fresh active-faction count.

## Admission gates, in original order

Let L be the runtime map layout scale, A region area, W the region wet count, R the retained region-density input, d global difficulty and H the candidate owner's human membership. Divisions truncate toward zero. These are turn-zero gates:

| Reject when | Condition |
| --- | --- |
| Search budget exhausted | n>1024 for a computer faction; the original turn-zero human path bypasses this refusal |
| Low elevation | Band<3 |
| Fungus | Live fungus bit set |
| Rocky | Rockiness>1 |
| Initial spacing | Distance<`trunc(6*L/56)` |
| High elevation | Band>=5 |
| Occupied | Live items low bits1/2 set and low affiliation nibble<8 |
| Polar | y<2 or y>=height−2 |
| Too few good tiles | Count<`12-trunc(n/80)` |
| Relaxed spacing | Distance<`trunc((16-trunc(n/64))*L/56)` |
| Early small region | n<900 and A<`36-trunc(n/64)` |
| Regional wet density | `trunc(W*(R+1)/2) < 24+(H ? 8*(6-d) : 8)-trunc(n/16)` |
| Region area | A<`48*(5-(H ? d : 3))-trunc(n/4)` |
| Shared region too small | Presence mask nonzero and `trunc(A/(popcount(mask)+1)) < 64-trunc(n/64)` |
| Site quality | Effective rating<`15-trunc(n/64)`, unless the native Look First rule bit bypasses this minimum |
| Landmark proximity | Bucket<`trunc((1024-n)/128)` or bucket==0 |

An excluded center has effective rating0. Otherwise use its nonzero cached rating; a zero cache calls the uncached query and setter. A returned0 is still written as0. In the seven stock searches, reached rejections cover elevation, fungus, rockiness, both spacing checks, good-tile count, rating and landmark distance. The other listed gates are source-backed alternatives; their rejection arms are not newly runtime-established by this baseline.

## Final refinement

Acceptance selects a search center, then a second inner21 scan chooses the final position. Keep only valid band3/4 tiles in the accepted region, without fungus or the occupied condition above. Require distance at least6 from existing Bases/Units. Preserve a source quirk: Base distance uses the refinement candidate, while Unit distances use the **accepted search center**.

Obtain each retained candidate's effective rating using the same exclusion/cache/miss rules. Add1 if any valid adjacent tile in the native eight-direction footprint is water. Update the best only for a strictly greater score; equal scores keep the earlier offset. Initial best is−1 and initial selected offset0. There is no added positive-rating requirement in this refinement.

| Faction | Generated pairs | Accepted center | Final Base/Scout position |
| --- | ---: | --- | --- |
| Gaia | 573 | 16,58 | 17,61 |
| Hive | 587 | 35,37 | 34,36 |
| University | 567 | 24,38 | 24,38 |
| Morgan | 493 | 59,23 | 59,23 |
| Sparta | 532 | 29,27 | 29,25 |
| Believers | 407 | 4,64 | 4,64 |
| Peacekeepers | 588 | 13,71 | 15,71 |

These coordinates are validation outputs, not startup constants. All three uncached queries occur during refinement and return0; their setters preserve affiliation. The initial all-map refill separately verifies3,200 ratings against the existing formulas, including positive values.

Scope is ordinary local turn-zero Standard Planet, human1 and computer2..7. Nonzero-turn restart, malformed/tiny geometry, human search exhaustion and the frontend entropy lifecycle are not expanded here. Native map-search scratch initialization executes; it is never treated as a display service. Verification: `tmp/reference/check_startup_site_admission.py --verify-only`. Accepted frozen evidence: `tmp/reference/startup-site-admission-evidence-v2-20261019/manifest.json`.

## Port implementation checkpoint

The C++ SmacStartingSite producer now independently matches all3747 native
candidates and measured metrics, rejected branches,65 refinements,3 cache
misses/setters and the seven complete final maps. Its existing Quality consumer
also reproduces all3200 initial refill values and128 regional wet counts from
the prepared terrain. These opt-in original replay checks consume private
terrain/metadata inputs, not expected candidate coordinates or recorded random
outputs. A separate synthetic control covers the source-mapped computer1025th
candidate refusal and the human bypass; it is not a new native exhaustion audit.

The input named region_density in the reference is the retained ocean-coverage
category at native0x9b952c, already owned by the map/settings ocean-category
producer. It is not derived from regional wetness.

The authenticated local SMAC default-world Start now clears/refills the owned
rating cache once, selects sites in simulation-owner order, and creates each
actual Base and Scout/Rover before selecting the next owner. This replaces the
provisional script picker for the currently admitted immediate-HQ local human
profile. It uses retained regional Base presence and the existing Base factory's
exclusion/cache history. Late event replays and cancelled admissions are rejected.
Post-selection balancing now follows these factories, including the mandatory
zero-deficit draw and real extra-Pod creation; see[startup-site-balance.md](startup-site-balance.md).
Look First and computer admission remain incomplete. SMAX/network startup still
uses its existing placeholder path.

Validation: the full private original replay and synthetic budget groups pass;
twelve focused actual default-start/research/lifecycle groups cover Gaian and
Spartan production, movement, next turn and save/restore. Targeted analysis of
five production translation units and focused ten-TU ASan/UBSan checks pass
(the remaining linked objects are uninstrumented; leak detection is disabled).
A fresh generated small-world Gaian game using the owned GOG assets reaches
research selection and the playable map, then exits through normal Menu Quit
with status0. This smoke is not a seven-faction Planet or visual-diff claim.
