# Sea-side transport mode0 boarding

This is the open-sea/no-Base-marker mode0 preprocessing used after ordinary
Base removal. It updates stored Unit records; it does not create a separate
cargo inventory or move boarded Units off their current tile.

## Capacity and traversal

Read capacity through the original capacity helper. It normally uses the
design's retained capacity byte. For qualifying predefined psi-weapon designs,
it instead returns the Unit's retained morale byte plus1. Both tested Transport
Foil and Isle report2 in this admission; those are observations, not constants
for every carrier. A zero-capacity Unit without the checked carrier ability
returns before boarding.

Mode0 has an additional **owner0 ordinary-capacity counter of99**. This is local
to this transport operation, not a change to the design or reported capacity.
The native control boards five Worms despite reported capacity2 and returns94.
The source also has a separate counter for its air-carrier ability; the tested
sea carriers lack that ability and do not board the air Unit.

Mode0 traverses the linked tile stack, from its head through next links, in two
passes:

1. Count/process eligible existing cargo whose order is1 and payload equals
   this carrier's dense Unit index. It consumes capacity before new boarding.
2. Use remaining capacity for eligible new cargo, in the same stack-link order.

The conventional capacity2 control starts with one existing cargo. Original
unlink/reinsert moves middle dense-ID Unit B to the stack head. B receives the
one remaining slot, ahead of candidates selected by either dense ascending or
descending order. Repeating mode0 when full returns0 and boards nobody new;
final Unit records are unchanged. Existing cargo is not added a second time.

## Candidate rules at the removed sea Base

Candidates must have **exactly the carrier's owner**. A positive-faction Pact
does not grant boarding: the tested foreign Pact Unit is skipped. Ordinary
land-domain candidates must themselves have zero transport capacity. Other sea
Units are skipped. Air Units require the separate air-capacity/ability path;
the tested ordinary sea carriers skip Locusts.

For new eligible cargo on this unsafe sea tile, the human-owner branch accepts
unordered land Units and order1 Units with negative payload. It leaves cargo
assigned to another carrier alone. The native/AI branch can reassign that
association: a native Worm initially assigned to a second Isle is boarded by
the first Isle and receives its index instead.

Additional source-mapped state gates matter when preserving complete records:
the AI branch also applies when carrier and candidate both have state mask
`0x200`. In that branch a candidate with mask`0x40000` is skipped unless mask
`0x20000` is also set and the owner's retained region-state byte is7. Without
`0x40000`, ordinary eligible AI candidates on this unsafe sea tile are admitted.
These special-state gates are source mapped; the new literal controls exercise
ordinary flags, not a full automation/region-state matrix. Safe Base/airbase and
air-carrier rules remain outside this bounded open-sea contract.

## Stored writes and index identity

Before scanning, mode0 clears state mask`1` across the whole linked stack,
including foreign Units. Each processed cargo receives:

- Order byte1.
- Signed payload word set to the carrier's **current dense Unit index**.
- The additional retained order word cleared to0.
- State mask`1` set after processing.

Coordinates, home, morale, the separate Unit flag word, and other opaque record
bytes preserve in these mode0 controls. Nonzero state mask`0x100` is preserved;
only mask`1` changes. Already assigned cargo also passes through the order reset,
so the diagnostic extra order word321 becomes0. No CRT/shared RNG calls or
seed changes occur, and Pact words preserve.

The payload is an index association only in this order context; it is not a
stable identity. Ordinary dense deletion repairs it. A reference above a removed
index decrements. If its referenced carrier is deleted, order clears to0 while
the numeric payload can remain stale. Do not normalize that inactive word to−1.

## Carrier deletion

The checked loss wrapper on old sea calls transport mode2, stages the assigned
cargo and carrier at the off-map coordinate `(-2,-2)`, then removes that stack.
The literal control deletes carrier then its waiting cargo. Another carrier,
that other carrier's cargo, the foreign Pact Unit, air Unit and Sea Former
survive. Their indices/associations compact correctly: the other carrier1→0,
and its surviving cargo's payload1→0 with order1 retained.

From the same boarded snapshot on old land, the wrapper removes only the
carrier. Its cargo survives at the original tile with order0 and stale payload0.
The other carrier association still compacts correctly. Both loss controls have
no RNG calls. Actual ordinary deletion accounting runs once for each casualty.
This is not a claim about over-capacity malformed cargo, air carriers, combat
outcomes or every loss-wrapper mode.

[Literal records, writes, capacity and deletion order](transport-mode0-boarding-cases.json).
Six complete native calls supplement the existing four Base-removal controls;
no gameplay changes or broad transport matrix were made.
