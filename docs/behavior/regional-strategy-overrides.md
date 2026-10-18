# RE-SMAC-REGIONAL-STRATEGY-002: codes0/2 and route inputs

This extends the [owner-summary producer](regional-research-summary-producer.md)
with33 complete native summary calls. Each supplies explicitly controlled
retained counters at the region phase, after the real preceding summary work;
they are **policy tests, not33 organically generated military distributions**.
Route, path and bilateral-policy helpers execute without supplied returns.
[Cases](regional-strategy-overrides-cases.json) record the inputs and results.

## Admission and retained input order

Keep the earlier entry0, empty-special-region and sea-region rules and the
initial7/8 pressure calculation. Its F input now means
[covered suitable](regional-lifecycle-and-territory.md), not Farm count.

For the current region define owner B=Base count, U=land-combat-unit count and
V=attack sum. For each faction1..7 in ascending order read peer C, u, v and the
owner's directed relation. Peer caches are not rebuilt for this query.

- Truce bit1 doubles peer C and u for the aggregate Base/unit totals; v is
  unchanged. Positive C contributes one to populated-faction count H, including
  self. Presence flags are set before later relationship admission.
- With C0, skip remaining peer work if u0 or V>5*v. Equality proceeds.
- Then skip self, truce bit1, or missing comm-link bit8.

Use the following S=strict and L=broad reachability for admitted peers.

## Fresh route query versus staggered retained policy

At turn<=1, use the fresh path below. At a later turn, use fresh routing when
**both** stored region area<500 and B*C<50. Otherwise calculate a phase:

| Stored area | Phase |
| --- | --- |
| Below1000 | `(turn+owner) mod8` |
| At least1000 | `(turn+2*owner) mod16` |

Phase0 uses fresh routing. A nonzero phase makes no route calls: S=L=false
when C>0 and the old strategy code>2; otherwise S=L=true. This reads the prior
strategy byte before this region's new7/8 result. Preserve thresholds500,
1000 and50, and the old-code2/3 distinction.

For fresh routing, C0 sets S=L=true without inspecting live Bases. Otherwise
query strict mode. Success sets both true and skips broad mode. Strict failure
queries broad mode and sets S=false, L=that result. A retained positive C with
no live qualifying Base can therefore differ from C0.

## Live route helper and the native path boundary

For a nonzero region, first require **both owners' retained regional Base
counts positive**. Otherwise return false before clearing traversal scratch.
Then clear the per-tile visited scratch and scan current Base records forward:

1. Select a current owner's Base in the requested stored map region whose
   tile is unmarked. Mark its contiguous raw-owner land territory component.
2. Scan peer Base records forward. Require the same stored map region and an
   unmarked tile; mark its contiguous raw-peer land component.
3. Ask the native route finder between these actual Base coordinates, with
   owner and peer identity, regional territory mode, the selected strict/broad
   policy and no supplied Unit. A nonnegative path result succeeds immediately.
4. Failed paths continue the scans, but component marks remain for this call.

This component suppression matters: two peer Bases in disconnected territory
produce two failed paths per mode; linking their raw territory produces one.
Do not replace this with every possible Base pair or a same-region=true rule.
The component flood follows land/raw-owner adjacency and horizontal wrap;
it does not require every traversed tile to have the requested region label.
Endpoint region filtering remains separate. Pure water seeds do not run that
land flood. Region0's no-count-gate entry exists but is not used by this pass.

The strict territory mode permits traversal of unclaimed, owner and peer
territory. The broad mode instead rejects unclaimed territory,
and can admit third-party owned territory subject to directed relationship
and human-owner checks. Thus broad is a mode name, **not a theorem that every
strict-legal path is legal in broad mode**. The caller never queries broad after
strict success. In the checked third-party controls, AI-owned intervening land
gives S=false/L=true; the same land owned by a human gives both false.

The route-finder call still depends on actual map/terrain and its other path
rules. These controls close this caller's inputs, endpoint/component selection,
mode dispatch and measured third-party cases; they do not certify a complete
arbitrary-map replacement pathfinder, sea-route behavior or all diplomatic
exceptions. Do not substitute Euclidean distance or a universal true result.

## Hostile and Pact contributions

Treat directed relation bits16,1024 or2048 as hostile for this phase. For each
hostile peer:

- Increment reachable-war count W if L. Count strict wars separately when S.
- If `(V<=u or V<=v) and U<=v and B>0 and L`, increment T2.
  Otherwise increment T0, **including an unreachable hostile peer**.

For a nonhostile Pact peer (relation bit32), collect a Pact mask only when
B>0 and v>U. If `V>u and C>0 and v<2*V`, OR mask2. Otherwise OR mask1 if L,
mask2 if not L. Preserve the strict and inclusive comparisons.

After the initial7/8 write:

1. If W0 and B>0, set regional flag256 and bypass both overrides.
2. Otherwise, T0>0 first **clears the entire Pact mask** and writes code0.
   If T0 is zero but Pact mask2 is set, write0 without clearing the mask.
3. Then T2>0 or the surviving Pact mask1 writes2.
4. Apply the existing final sea-region>=64 force7 unless the earlier
   empty-special shortcut has already returned8.

This corrects a detail omitted from the earlier partial description: a Pact
mask1 cannot rescue code2 after a hostile T0 clears it. Two hostile peers with
one T0 and one T2 do end at2, because actual T2 still wins after code0.

## Region flags and unrelated bilateral policy

The owner-summary reset retains only flag8 except its sixteen-phase expiry;
the territory producer preserves these flags. In this peer phase, OR:

| Condition | Flag value |
| --- | ---: |
| Own retained Base presence | 16 |
| Other retained Base presence | 32 |
| Presence with directed Vendetta bit16 | 64 |
| More than one strict reachable hostile peer | 512 |
| v>=trunc(3*V/2), B>C and L for a hostile peer | 16384 |
| Reachable human hostile owns a same-region Base previously owned by this owner | 1024 |

There is also flag4: for a relation without bits1/2, set its contributing
predicate directly; otherwise require either directed native bilateral policy
predicate and L. Truce peers have already skipped this stage. This predicate
does **not** feed W, T0, T2, Pact mask or initial pressure, and therefore is not
a missing code0/2 input. Its broad AI diplomacy calculation remains separate;
do not invent zero policy results for other consumers. Additional flags from
the earlier Base/unit stages are outside this table.

No RNG calls were observed in the33 controlled summaries. Reuse the earlier
1,792 all-region strategy comparisons for the admitted opening rather than
replacing live retained producers with fixed7 or zero foreign inputs.
