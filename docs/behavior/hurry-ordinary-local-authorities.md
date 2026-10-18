# Hurry: ordinary-local blocking and retool authority

RE-SMAC-HURRY-AUTHORITY-001, 2026-10-18. Pinned stock SMAC. This closes two
inputs of [Hurry](hurry-production.md); its quote/payment arithmetic is reused.
[Eleven native controls](hurry-ordinary-local-authorities-cases.json) execute
the installed RULES loader and admitted turn/positive-choice fragments.

## Positive-choice blocking state

The guard reads **secondary runtime state mask `0x00000800`** after a positive
dialog choice and before the partial-payment marker. It is a transient
turn-advance/input-lock state, not a selectable Standard/Current scenario rule,
HURRY multiplier, partial-payment marker or retool option.

A source-identified producer is the network handler's **server NEXT TURN**
message (`0x4301`). In the checked non-simultaneous network branch it sets that bit and a
separate next-turn latch. The native turn driver clears the bit after its
upkeep boundary and on the traced local-turn reentry/continuation paths.
The ordinary new-game producer retains only the two randomization preferences
in this secondary word; it clears this blocking bit.

For the supported fresh ordinary local campaign, **blocking=false**. Both full
and partial positive choices pass this guard with the bit clear, and both
take the no-purchase route with it set. A poisoned startup control containing
the bit clears it while preserving the randomization preferences. Existing
continuous Standard→new-game controls independently establish that clear.

Do not expose a fictitious “blocking mode” option to the player or infer this
bit from the primary rules word. The separate multiplayer “declared turn
finished / Waiting for Others” state uses a per-player mask; this mapping
does not identify that mask with this server-advance bit. Complete multiplayer
locking/message admission remains outside this ordinary-local closure. The
configuration identity is source-backed: the command UI displays its
“Simultaneous Movement” requirement notice when this configuration bit is set.

## Active retool exemption

Hurry reads the **current active RULES retool exemption scalar** directly.
The common rules loader loads it from the “Retool exemption (first X minerals
not affected by penalty)” row, after retool penalty percentage and strictness.
The native numeric read/clamp bounds are **0 through 1000**. The installed
stock `alpha.txt` value is **10**, confirmed by executing the original file
lexer, numeric parser and RULES loader after poisoning the scalar to 777.
The active stock rules basename is ALPHA. This is SMAC evidence, not an
`alphax.txt`/SMAX value.

There is no faction, difficulty, world-size, INDUSTRY or current-item lookup
between that scalar and Hurry's comparison. Compare **stored minerals < active
exemption**; equality avoids this doubling. Do not scale the exemption by the
mineral factor or substitute the separate early-project threshold of four
times that factor. The same active scalar also serves production switching
and initial mineral posting, whose other rules remain their own contracts.

For the admitted installed stock ordinary-local profile, the resolved inputs
are therefore **blocking=false, retool_exemption=10**. The already-established
strict startup HURRY default100 and absent Voice project are unchanged.
Custom/restored authorities must retain their explicit supported provenance;
this receipt does not certify every custom rules path or save frontend.

The eleven controls make no gameplay/CRT RNG calls. They supply external file
services and admitted fragment registers; they do not execute the full Hurry
dialog, complete turn controller, network delivery or payment UI.
