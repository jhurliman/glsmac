# Manufactured Unit morale, flags and allocation refusal

Ordinary manufactured land units receive High Morale **after** local training's
negative-current-MORALE halving. The first-prototype point follows High Morale;
the baseline point follows that; a single final clamp writes stored morale.
The normal produced-Unit flag is unconditional after successful allocation.
At the 2048-unit limit, production retains its mineral debit and prototype
history changes but leaves the current target and pending queue in place.

Authority: GOG `terran.exe`, SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
See [17 complete Base-return receipts](design-workshop-newborn-state-cases.json)
and the [self-authored scalar reference](workshop_newborn_state_reference.py).
This extends [first-prototype production](design-workshop-first-prototype.md)
and [ordinary land training](ordinary-production-training-inputs.md).

## Stored morale and ordering

For the ordinary nonnegative-offense land branch:

```text
training = 2 * (Command Center OR owned Command Nexus)
         + 2 * (Bioenhancement Center OR owned Cyborg Factory)
if current faction MORALE < -1:
    training /= 2

stored = clamp(training + HighMorale + FirstPrototype + 1, 0, 6)
```

Each boolean bonus contributes zero or one. The training total is nonnegative
and even before division. The current-MORALE threshold is strict: −1 does
not halve training; −2 does. Positive current MORALE does not add points in
this helper. Retained MORALE and later effective-morale adjustments are
separate consumers.

`5059b0@50660d` calls original `4fbc90(base,owner,domain)`. Its land branch
tests facilities 27 and 30 and the hosting-Base ownership of Command Nexus
(`95a020`) and Cyborg Factory (`95a060`). Each facility/project pair supplies
at most two. At `4fbe36..4fbe47`, current MORALE from
`95d404 + owner*20bc` controls the halving of the combined training result.
Facility/project dependency and duplicate-grant controls remain the earlier
training receipt; this new packet exercises actual local facilities.

The caller then performs these original operations:

| Address | Operation |
| --- | --- |
| `50661e` | `5d6310(design,4000)` checks the produced design's High Morale bit. |
| `506626..50662a` | If set, increment training accumulator EDI once. |
| `50662b..506633` | If the saved first-completion boolean is nonzero, increment EDI once. |
| `506634` | Increment EDI for the ordinary baseline. |
| `506635..506649` | Clamp the complete total to 0..6. |
| `506658` | Store byte Unit `+24` (hex offset). |

`5d6310` is a direct design-mask test at Design `+20`; it does not query
technology or add another faction modifier. FirstPrototype is saved from the
old custom Design prototype bit four before the allocator runs. It is not
derived from unit count, training, mineral surcharge or High Morale. Allocation
initialization's earlier stored morale is overwritten here.

Representative complete custom Infantry/Hand Weapons/Synthmetal/Fission
productions, with High Morale created through the original design builder:

| Local training | Current MORALE | High Morale | First prototype | Training result | Total | Stored |
| --- | ---: | --- | --- | ---: | ---: | ---: |
| None | 0 | No | No | 0 | 1 | 1 |
| None | 0 | Yes | Yes | 0 | 3 | 3 |
| Both facilities | 0 | No | No | 4 | 5 | 5 |
| Both facilities | 0 | Yes | No | 4 | 6 | 6 |
| Both facilities | 0 | Yes | Yes | 4 | 7 | 6 |
| Both facilities | −1 | Yes | Yes | 4 | 7 | 6 |
| Both facilities | −2 | Yes | Yes | 2 | 5 | 5 |
| Command Center | −2 | Yes | No | 1 | 3 | 3 |

The full receipt includes all first/repeated × High-Morale/no-High-Morale
combinations with no training, both facilities at MORALE zero, and both at
MORALE −2. It additionally checks the −1 boundary, one facility at −2, and
the flag conditions below. No training/ability/allocator result is supplied.

## Immediate produced-Unit state stores

These offsets are relative to the 52-byte Unit record. Preserve existing
bits: each flag update is an OR, not a replacement.

1. For a human producing owner (`959ff0` owner bit), if producing Base `+38`
   has **both** `80000000` and `80`, `5064ac..5064af` ORs Unit `+04` with
   `00000200`. `5064b5` also writes zero to Unit byte `+26`. This is earlier
   than the ordinary morale calculation. The control with Base flags
   `80000080` observes the actual store; neutral controls bypass it.
2. After storing morale, `50665e..50666e` ORs Unit dword `+04` with
   **`00002000`**. It does this for first and repeated production, with and
   without training or High Morale, independently of the local/current faction.
3. If produced owner equals global `9430d4`, `506676` ORs **Unit byte `+09`**
   with `10`. This is a separate field from the dword at `+04`.

Neutral controls finish with Unit `+04=2000`, byte `+09=00`. Current-faction
controls have byte `+09=10`; the additional Base-flag control finishes with
`+04=2200`. The packet captures complete Unit records immediately after
allocation and around each of these stores, plus the record after the full
Base return. It does not assign speculative UI/order meanings to these bits.

Successful newborns retain design 127, owner one, home Base zero, and zero
damage, order and movement-spent bytes. The earlier unit of the same design
at a different square remains byte-identical. Same-square stack-link changes
are ordinary insertion and remain visible in the private record differences.

The conditional Colony population/Base-state path and negative-offense native
morale path are separate source branches. This packet does not generalize
the ordinary morale formula to those branches or establish AI-specific policy.

### Viewer identity and governor word authority

`9430d4` is the **primary map view's viewing/player faction**:
`91f500 + 23bd4`. It is the same field used by map disclosure, picking and
own-Unit activation; see [actor activation](actor-activation.md). It is not
the owner of whichever Base is being processed or an alias of the faction
upkeep cursor. The `selected_owner` input name in the newborn receipts means
that the produced owner matches this viewing faction.

Relevant producers and lifetime edges:

- New-game setup `5a0b80` writes the selected faction to `9430d4` at
  `5a14b6`, alongside construction of the corresponding human mask at
  `5a14bc`. Its network/multi-human selection branch also writes the chosen
  human at `5a0c3f` or `5a1ed8`.
- The ordinary turn scheduler `53be70` loops owner EDI through factions. At
  `53bf92` it writes processed owner to **`95a2ac`**, while retaining
  viewing faction `9430d4`. The human test `53bf8b..53bf98` sends an AI owner
  directly to `53cabc`, bypassing the viewing-context handoff. A human context
  switch can set `9430d4=EDI` at `53c060` before the UI/command continuation.
  A human already matching the viewer can bypass that handoff as well.
- Game codec `5be250@5be42c..5be468` saves/loads the DWORD at `9430d4`.
  Post-load `5c0941..5c0946` copies that viewing faction into resume cursor
  `9ba55c`; the copy does not make the two fields one authority. See the
  [load continuation](firstbase-startup-schedule.md#completion-and-saveload-suppression).

Therefore use the port's explicit **local viewing/player participant** for
the newborn byte-`+09` comparison in an ordinary single-local-player game.
Do not substitute the multi-owner upkeep iterator: otherwise every processed
owner would receive this local-view bit. If adding human/hotseat context
switching later, update the viewing participant at that context boundary;
do not treat it as immutable across setup, save/load and human handoffs.
These source edges do not establish every editor/network/transient-view mode.

The Base `+38` load in `50648f` is the same persisted **governor-options
word** used by worker allocation and the existing Base resource context:

| Mask | Governor option |
| --- | --- |
| `80000000` | Governor active |
| `00000040` | Manage citizens and specialists |
| `00000080` | Automate new units |

Advanced-governor conversion reads that exact Base word at `41fd2f`, then
tests `80000000`; `41fd5c..41fd6a` maps its `80` bit to option index two;
`41fd6f..41fd7d` maps its `40` bit to option index three. The established
[settings contract](governor-settings-and-worker-reset.md) names those
independent options. The newborn `00000200` guard is consequently
**human owner AND GovernorActive AND AutomateNewUnits**. ManageCitizens is
not consulted there. Pass the existing full `governor_options` word; do not
introduce a second Base flag authority for this consumer.

[Three scheduler fragments and six governor conversion controls](design-workshop-newborn-context-cases.json)
pin the distinction without another production/raster matrix. With viewer
one and processed AI owner two, scheduler owner becomes two while viewer
remains one. With two admitted human contexts, the handoff store changes the
viewer to two. Private additive bundle:
`tmp/reference/independent-workshop-newborn-context-evidence-20261021`,
manifest SHA256
`7f7b000260a9f86ad8b0cc1c8bf12ac478b871e1e64b9f39476dc8b8177e218a`.
The fragments stop before UI/upkeep calls; the human-store control supplies
the preceding UI stage and does not execute a complete hotseat transition.

## Unit-count 2048 refusal tail

The allocator `5d8520@5d8529..5d8539` returns −1 when global `959fd0 >= 800`
(hexadecimal). It exits before incrementing the count or initializing a Unit.
Production has already completed prototype history and debited minerals.

At `506395..506397`, a negative allocator result takes `50639d`. That branch
only cleans up temporary dialog/storage objects, then jumps at `50642f` to
`5071c4`, performs final cleanup and jumps at `5071e7` to the normal production
epilogue `50924e`. There is no mineral-refund or prototype-rollback helper
on this tail. It bypasses newborn morale/flags, the success-side Base `+30`
flag updates, and human queue advancement `5058c0` at `5067f2`.

The caller `50c760` continues after production at `50c7cf`: normal Base
aftermath executes, mineral baseline `+98` is updated from store `+40`, target
baseline `+44` from current target `+50`, and the later resource pass executes.
Thus allocation refusal does not abort the complete Base return.

Two new controls execute this entire return with a supplied sparse global
count of 2048, both training facilities, current MORALE −2 and High Morale.
The first starts with Design flags 3 and the repeated control with flags 7.
The actual original builder/cost/resource pass gives these results:

| Refusal | Minerals before resource increment | Charged | Store/baseline after | Design flags after | Target / queue |
| --- | ---: | ---: | ---: | ---: | --- |
| First prototype | 39 | 40 | 0 / 0 | 7 | 127 / pending Scout preserved |
| Already prototyped | 29 | 30 | 0 / 0 | 7 | 127 / pending Scout preserved |

Each resource pass supplies one mineral before the debit. Both returns are
zero. Current target remains 127, pending count remains one, its entry remains
design two, and stored target baseline becomes 127. Full Design history
matches the independent prototype-history reference, including inferred
design flags. The count remains 2048 and **all 2048 Unit records** remain
byte-identical across each refusal return. No training or newborn flag stage
is entered and no success queue-advance call occurs.

These are controlled sparse-count fixtures, not a naturally played map with
2048 valid units. They close rollback/refund/queue behavior for this tail
through the complete outer return; arbitrary carryover, governor and AI
queue behavior are not newly covered.

## Evidence scope

Private additive bundle:
`tmp/reference/independent-workshop-newborn-state-evidence-20261021`.
Manifest SHA256:
`c6e44e15a2d36876fa568fca0fb3933c14af2de9270e23697eb173de2a58f257`.
The bundle retains 17 complete `50c760` returns: 15 successful newborns and
two allocation refusals. Native ordered writes and full input/output Design,
Base, Unit and Faction buffers are private; public cases contain scalar state
and hashes. The two refusal controls additionally retain the full Unit array.

Execution inherits the earlier headless movement/visibility and explicitly
listed notification/destructor/deallocation services. It supplies the same
zero foreground-view presentation flag only around the established path-query
tail, restoring the pointer immediately afterward. Facilities and current
MORALE are declared input grants. No interactive executable is launched, and
no gameplay helper, cost, training, history or queue result is substituted.
Earlier evidence bundles are unchanged. Production code and Git references
are untouched.


The [Colony limit supplement](design-workshop-colony-limit.md) separately
captures size two, easy size one, accepted abandonment and Delay. Refusal
precedes population loss/Base removal, while the size-one decision precedes
the debit and allocation attempt.
