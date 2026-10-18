# Ordinary Base-removal center Unit cleanup

Use the ordinary Base-removal transaction for starvation and flood deletion.
Its center cleanup reads the **current retained surface class of the lost
Base tile**, not the class the climate caller will write later. In the flood
caller, that final class write occurs after Base removal returns.

The order is:

1. Repair homes: own Units homed at the lost Base select the nearest remaining
   own Base, or are removed if none remains; foreign lost-home links clear.
2. Clear the Base marker, compact Base records, and repair home/Base references.
3. Read old surface and perform the following center cleanup.
4. Finish ordinary radius/territory/presentation cleanup and return. A climate
   caller can then perform its separate incompatible-domain pass and class write.

| Old surface | Center cleanup |
| --- | --- |
| Land, class≥3 | Traverse Units backward; remove **sea-domain1** Units |
| Sea, class<3 | First traverse transport-plan7 Units forward and call the original transport helper in mode0; then traverse backward and remove **land-domain0** Units unless order is1 and its signed payload is nonnegative |

Domain comes from the design's chassis, not from the design ID or plan. Plan7
only selects the sea-side transport preprocessing. Air-domain2 Units are not
selected by either casualty predicate. The casualty test does not restrict
ownership or home status. Home repair has already happened, so a Unit can be
rehomed and then die in center cleanup. The climate class-conversion flag`0x20`
exemption does not exempt a Unit from these removal predicates.

The numeric position test is `(wrapped_abs_dx + abs_dy) & ~1 == 0`. With valid
parity coordinates this means the center; an adjacent valid radius tile has
distance2 and does not qualify. Preserve the source distinction if projecting
coordinates differently in the port.

Four complete removal controls establish the missing discriminators:

- **Old land, mixed roster:** casualty order is Sea Former design4, Transport
  Foil3, then native Isle9, following reverse dense order. Native Worms8,
  Locusts10, own Scout2 and an off-center Sea Former survive. The Isle has flag
  `0x20` and a foreign lost home; its home clears before it is removed. The own
  Transport is rehomed before removal. Sea Former plan9 proves the casualty
  predicate is sea domain, not transport plan.
- **Old sea, mixed transports:** native Isle9 and own Transport Foil3 receive
  mode0 preprocessing, in forward order. They actually board the previously
  unordered native Worms and own Scout, respectively, setting order1/payload
  to their transport IDs. All seven Units survive; there is no debit.
- **Old sea, no transport:** own Scout2 is removed before native Worms8. Sea
  Former4 and air Locusts10 survive; the off-center Sea Former survives too.
- **Old sea, order discriminator:** order1/nonnegative payload land Units survive
  the direct predicate. An order1/negative-payload Scout and flag`0x20` unordered
  Worm are removed, in reverse order. The nonnegative payload here is an explicit
  numeric canary, not a claim that the target is a valid transport.

The sea transport helper contains a valid-Base test using Base presence plus
cached affiliation0..7. At this center-cleanup stage the removed Base marker is
already clear, even though cached affiliation still says1. That former Base
therefore provides no Base exemption. Earlier lost-home removal calls occur
before marker clearing and must retain their original helper context.

Casualties call the ordinary loss wrapper, which reaches ordinary Unit deletion
in these controls. Each deleted Unit debits its active-design byte once; a
plan≤7 also debits the active-plan tally once; global roster count decreases.
Plan9 Sea Former receives no active-plan debit. Native counters are deliberately
seeded17/1000: Isle death gives design9=16/tally999; Worm death gives
design8=16/tally999. Boarding-only cleanup leaves all three native bytes17 and
tally1000. This agrees with the [existing accounting contract](native-deletion-accounting.md).

[Literal orders, homes, counters and survivors](Base-removal-center-cleanup-cases.json).
The declared second Base and factory-created mismatched-domain stacks are
bounded producer admissions. This does not claim a full transport/combat matrix
or resume the broad RE goal.
