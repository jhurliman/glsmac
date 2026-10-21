# Fresh-image authority for retained air counters

**A newly allocated fresh world may initialize all three air counters to
known zero, without a recount.** This reproduces the pinned executable's
fresh-image storage authority and avoids making the first-session presets
unavailable merely because `5773c0` has not run yet.

All24 addresses, the three fields across owners0..7, occupy the zero-filled
virtual tail of the PE data section. Section RVA`0x2a6000`, virtual size
`0x3247f8`, raw file-backed size`0x21000`. For owner0 the section-relative
offsets are`0x2b79ec` (`95d9ec`), `0x2b922c` (`95f22c`), and`0x2b9230`
(`95f230`). All owner-stride additions remain in this zero-fill range.
[Scalar PE checks](air-summary-fresh-image-cases.json) pin all24 addresses.
This is initial image storage, not a call to calculate current live counts.

There is also an **explicit turn-zero initializer clear for the two own-air
fields**: the `5c8d5e..5c8d6e` loop clears`95f214..95f238`, including
plan0/plan4. It does not cover foreign air`95d9ec`; the earlier poison
control's17 survives while its own-air19/23 clear. These facts agree:
fresh storage begins zero, but the initializer does not reset every field.

Nonzero restart jumps`5c89ef → 5c8da5` and bypasses that own-air clear.
Preserve all three retained values across the supported restart boundary.
An absent legacy-save field is not fresh storage and should remain absent
until a genuine authority source is available. Reused-process New Game
with arbitrary old faction state is likewise not proved to clear foreign
air by this receipt. Do not generalize initial PE zeros into a universal
new-game, load, or recreation reset.

For the port: give newly created clean faction state a present summary
tuple `(foreignAir=0, ownPlan0=0, ownPlan4=0)`. Continue retaining that tuple
through ordinary startup, with no extra final recount; the first admitted
post-Base owner summary later replaces it. Existing loaded/restart state
retains its separate provenance. See [checkpoint order](air-summary-checkpoint-order.md).

Private additive freeze:
`tmp/reference/independent-air-summary-fresh-image-evidence-20261021/`,
manifest SHA256
`7346d148716652b47213c3a902e5d4dc64604e63935d4f002d4568768535becc`.


## Live integration

Game instance storage now starts with present zero triples for all8 slots.
Actual Reset captures retained rows before releasing the simulation. New-world
preparation carries those rows forward, clearing only own-plan0/plan4 for each
admitted turn-zero owner; foreign history, peers and missing rows survive.
The simulation default and decoder remain unchanged, so legacy loads retain
absence and restore does not acquire cold-storage authority. No new recount
or save-format change is added.

The real Reset/new-simulation fixture verifies cold zeros, retained distinguishing
values for every slot, participant-only own-counter clears, an absent peer and
all-absent legacy history. Existing actual Start, saved research continuation,
restart/elimination and owner-summary fixtures cover the adjacent boundaries.
All73 selected native groups pass normally. Targeted correctness analysis is
clean for8 production translation units/consumers. All24 public PE scalar cases
were independently verified against the owned executable SHA and actual section
headers. All73 groups also pass focused ASan/UBSan with10 production and4 test translation
units instrumented (leak detection disabled).
