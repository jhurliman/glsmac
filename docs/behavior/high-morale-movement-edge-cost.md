# High Morale and ground edge cost

2026-10-21. Contract `RE-SMAC-HIGH-MORALE-EDGE-001`.
Pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Native scalar receipts](high-morale-movement-edge-cost-cases.json).

**High Morale (`0x4000`, bit14) does not change the complete ground edge-cost
query.** An otherwise supported Infantry or Speeder design must not be refused
by that query merely because this ability is present. This narrowly extends
the [existing edge-cost contract](../smac-ground-edge-cost.md); other abilities retain
their own rules and scope.

The original complete query at `5aaad0` tests Antigrav Struts (`0x400`) through
`5d6310` at `5aad27`. Its fungus branch can call basic design allowance
`5d9400` at `5aadc7` and `5aadd4`. For ground designs that helper tests Slow
Unit (`0x800000`) and Antigrav (`0x400`); its additional air branch tests
`0x20`. The predicate at `5d6310` masks the supplied bits directly against the
design ability word. Neither routine tests `0x4000`, nor does the edge query
read a unit's morale, damage or spent movement. Full private source extracts
and the executed predicate-call journal accompany the evidence packet.

The existing original-x86 fixture supplies the same conventional custom design
64, faction1, Q3, reactor1, no project, ordinary terrain switch0, and adjacent
doubled-coordinate step `(8,8)` to `(9,9)`. For each chassis/terrain pair only
the ability word changes from0 to`0x4000`. All **16 paired cases / 32 complete
edge queries pass**:

| Terrain | Infantry, either ability word | Speeder, either ability word |
| --- | ---: | ---: |
| Flat | 3 | 3 |
| Rocky destination | 6 | 6 |
| Forest destination | 6 | 6 |
| Connected road endpoints | 1 | 1 |
| Connected diagonal river endpoints | 1 | 1 |
| Connected tube endpoints | 0 | 0 |
| Destination fungus, retained PLANET0 | 9 | 9 |
| Destination fungus, retained PLANET+1 | 3 | 6 |

The last row deliberately exercises the raw basic-allowance branch. The
ordinary edge cases are the requested seven terrains; this one extra pair per
chassis distinguishes a hidden speed dependency without expanding to a new
terrain/ability matrix. No native helper return or instruction is replaced.

Basic allowance and elite movement remain separate. Eight additional supplied
neutral, homeless, undamaged unit states execute the complete basic, effective
morale, full allowance and elite-suppressed allowance queries. Both ability
words produce these same results:

| Chassis | Stored/effective morale | Basic allowance | Full allowance | Elite bonus suppressed |
| --- | ---: | ---: | ---: | ---: |
| Infantry | 5 | 3 | 3 | 3 |
| Infantry | 6 | 3 | 6 | 3 |
| Speeder | 5 | 6 | 6 | 6 |
| Speeder | 6 | 6 | 9 | 6 |

The unit allowance `5d9530` queries effective morale with `(unit,1,0)` and
adds Q when that result is6 and the caller does not suppress the elite bonus
(`5d95ae`–`5d95f4`, with the existing predefined-Psi exception). High Morale
can affect training elsewhere; these controls supply stored morale and do not
establish a production/training or promotion producer. Preserve that boundary
when using the [existing full allowance rules](../smac-movement-rules.md).
In particular, the positive-PLANET fungus edge uses raw basic allowance,
not the elite unit's larger full allowance.

This establishes query behavior, not complete move legality/admission,
movement spending, combat, other abilities or a booted campaign. Private
probe, source extracts and receipts are frozen under
`tmp/reference/independent-high-morale-edge-evidence-20261021/`.
