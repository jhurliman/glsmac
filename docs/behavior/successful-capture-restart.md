# RE-SMAC-SUCCESSFUL-CAPTURE-RESTART-001: successful last-Base capture restart

One continuous original-game execution captures a human faction's final Base,
successfully searches another land region, creates the replacement package,
performs the post-restart tracking reveal, returns from faction disposition,
recalculates the captured Base, and enters its screen without supplying an
answer. [Cases](successful-capture-restart-cases.json) contain the exact states,
creation order and RNG transitions. This joins the earlier separate
[capture](last-base-capture.md), [search](restart-search.md) and
[package](restart-unit-package-selector.md) contracts.

## Admitted setup

The fixture uses turn 6, Librarian difficulty, local human captor 1 and losing
faction 2, both alive and human. The map is horizontally wrapped 64×32 with
clear flat rainy land at `4 <= x < 32` and `36 <= x < 60`, separated by shelf
water. Actual region-fill and terrain-postprocessing routines produce land
regions 1 and 2, with stored areas 448 and 384 and fertile counts 420 and 360.
This is fixed logical terrain, not a generated-world or chosen-seed claim.
The complete metadata is in the cases, including retained polar accounting.

Captor Base 0 is at (16,16); target Base 1 is at (20,16), population 2 and
undefended. A native Scout moves from (19,15) into the target. Before capture,
faction 2 also has a Former at (30,16), home Base 1, and Scout at (24,12),
home −1. The latter is created by the original factory, which initially assigns
home 1; its home is explicitly supplied as −1 before the run. Both factions
retain technology 6. Credits start at 100/200; starting-energy rule is 10.
CRT and shared RNG states both start at 3. No search result, accepted site,
restart outcome or replacement design is supplied.

The supplied retained regional Base counters are deliberately stale:
captor region 1 = 3, loser region 1 = 2, and region 2 absent for both.
They remain unchanged throughout this capture/restart. Live Base ownership
changes independently from one Base each to two for the captor and none for
the loser. Do not silently rebuild the regional counters from live ownership.

## Capture, cleanup and successful placement

Capture changes credits to 233/67 and the target to captor-owned population 1.
Home repair removes the old homed Former. The old unhomed Scout survives home
repair, then faction disposition removes it before entering the initializer.
The initializer adds 10 credits to the loser, giving 233/77.

| Attempt | Candidate | Region | Result | CRT state after coordinate pair |
| --- | --- | --- | --- | --- |
| 1 | (29,13) | 1 | Fails relaxing Base-distance gate | 1716616744 |
| 2 | (44,8) | 2 | Selected | 1772436962 |

Region 2 has no nearest Base and no faction-presence bits. The caller uses
distance 32 for the missing Base; landmark proximity is the no-hit sentinel
32. The reached site-quality read reuses cached rating 1 without a query or
cache write. The first candidate fails the distance gate: this run does not
demonstrate a captor-region veto rejection. See the earlier failed capture
receipt for that case.

All four original factories return successfully, in this exact order:

| New Unit ID | Design | Owner | Position | Home | Stored morale | Effective morale |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | Colony Pod (0) | 2 | (44,8) | −1 | 1 | 0 |
| 3 | Former (1) | 2 | (44,8) | −1 | 1 | 0 |
| 4 | Colony Pod (0) | 2 | (44,8) | −1 | 1 | 0 |
| 5 | Scout Patrol (2) | 2 | (44,8) | −1 | 1 | 0 |

Damage, movement spent and order are zero for each replacement. Effective
morale is separately queried after stopping at the screen, with guest context
restored afterward. Initializer returns success 1 to disposition. The old
units were deleted before these new IDs were allocated; reused IDs do not
mean that the old units survived.

## Post-restart tracking reveal

Successful local disposition has an additional CRT draw when the captor is
the local focus faction and network mode is off. It occurs after placement
and package creation. This draw must be retained even if the port already
implements placement RNG and ordinary new-unit sight.

Read the retained bilateral reveal modifier from local focus toward the loser.
The primary relation mask is **`0x1000` (4096)**: set gives bound 2; clear gives
bound 4. This case's retained primary relation word is 16, so the mask is clear.
The semantic identity
of that diplomacy modifier is unresolved; do not infer it from faction names
or ordinary treaty status. The bound-2 alternative is established statically;
this execution observes the clear modifier and bound 4.

Consume exactly one CRT draw and reveal only when `rawCRT % bound == 0`.
Nonzero remainder skips tracking. Here the transition is
`1772436962 -> 1632431389`, raw output 24908, remainder 0 for bound 4.
The shared RNG is untouched. The draw does not select a random Unit index.

After a zero remainder, choose the losing faction's nearest Unit to (0,0),
excluding no Unit. Scan IDs ascending and replace on equal distance, so later
IDs win ties. For wrapped absolute horizontal distance dx and vertical
distance dy, use the integer metric:

```
max(dx,dy) - ((dx+dy)//2 - min(dx,dy) + 1)//2
```

All four replacements are at wrapped distance 17, so Unit 5 is selected.
No eligible Unit means no reveal. Reveal terrain to **captor 1**, not to the
restarting faction 2, at the center and eight immediate neighbors in order:

```
(44,8), (45,7), (46,8), (45,9), (44,10),
(43,9), (42,8), (43,7), (44,6)
```

The general loop wraps x on wrapped maps and skips invalid coordinates,
including out-of-range y. It invokes the existing terrain-revelation operation:
add recipient exploration and refresh that recipient's remembered terrain.
This case has no Base in those nine squares. Base-discovery side effects of
that generic operation are outside this case.

The generic reveal initially grants captor visibility to every Unit on the
selected square. Then walk that square's linked Unit stack. Clear every member's
visibility mask, and give the captor visibility on only the first Colony Pod
encountered. The actual traversal is 4,5,2,3; Unit 4 receives mask 2 and Units
5,2,3 finish at zero. This is a stored visibility change in addition to terrain
exploration, not visibility of every replacement. Exact terrain mask and
remembered-terrain deltas are recorded in the cases.

### How creation becomes stack traversal

Use returned Unit identities, not their numeric sort order. On a fresh empty
restart square, each factory inserts its new Unit at the front of the linked
stack. After creating the initial Pod and Former, the initializer explicitly
moves that Former to the back. It then creates the second Pod, defender and
any turn-dependent additions. After all those grants, it explicitly moves the
second Pod to the front. A coordinate stack selector finds any matching Unit
by ascending ID, then follows predecessor links to the actual front; tracking
also follows predecessors from its independently selected nearest Unit before
walking successors. Selection of Unit 5 therefore does not make Unit 5 the
first visited member.

For successful factory returns in creation order `C1,C2,C3,...,Cn`, with
`C1 = initial Pod`, `C2 = Former`, `C3 = second Pod`, the resulting fresh stack is:

```
C3, Cn, C(n-1), ... , C4, C1, C2
```

Here creation IDs `2,3,4,5` become `4,5,2,3`. If the same package has three
additional Scouts created afterward inside the initializer, creation IDs
`2,3,4,5,6,7,8` become `4,8,7,6,5,2,3`. This seven-unit order is a static
consequence of the same producer and final promotion, not another native
capture observation. Optional mobile/balanced/defender grants follow their
actual creation identities in the same formula, regardless of design.

The formula covers the existing nonzero-turn package on an initially empty
square, successful allocations, and no intervening stack mutation. A Scout
created by the ordinary factory **after the initializer returns** instead
goes ahead of the current front; do not apply the initializer's final Pod
promotion a second time. Occupied sites, failed allocations, loaded/repaired
stacks and later movement/reordering require their actual linked state. This
receipt does not establish their order from a sorted occupant list.

## Ordering, RNG and limits

| Phase | CRT draws | Exit CRT state |
| --- | --- | --- |
| Capture before restart search | 37 | 1951609278 |
| Two placement coordinate pairs | 4 | 1772436962 |
| Post-restart tracking | 1 | 1632431389 |
| Total | 42 | 1632431389 |

Shared RNG remains 3 with zero draws. All four replacements already exist
before captured-Base recalculation and resource calculation. Disposition
returns 1; those calculations return before captured-Base screen entry.
Active/human masks remain 6, focus/controller/active faction remain 1,
ending state remains 0, and the retained game flags remain 16384.

The headless harness supplies pre-existing display acknowledgments and one
exact tracking-recenter presentation call whose main-view receiver is null.
Its return value is unused. Native search, RNG, factories, reveal, stack
visibility changes and resource calculation execute normally. The original
null-view renderer diagnostic is preserved separately, as is the later
instrumentation-only failure before its register-constant fix. Neither is
used as proof of screen entry; the final successful trace supplies that proof.
The screen is stopped at entry without answering: screen closure, capture
return, outer movement return, actual rendering, AI/network alternatives and
the bound-2 runtime branch remain untested by this fixture.
