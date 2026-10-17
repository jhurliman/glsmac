# Canonical local turn upkeep

The native local `complete_turn` request now binds the incoming turn number when
queued. Dispatch admits only the actual local Single controller and the admitted
Single/Player human roster, checks exact current+1, and processes factions in
ascending accepted simulation-ID order, then saved Base record order within each
owner. This faction controller order is an explicit GLSMAC local policy; the
original receipt proves the per-faction traversal. Duplicate queued requests are
ignored as stale and cannot run upkeep twice or discard a following command. Direct
script `advance_turn` cannot bypass the native route when worker knowledge exists.

The transition refreshes social state and seals pending startup Units, publishes
the internal incoming turn, resets conventional spent movement, and calls the
complete `Base::ProcessSmacUpkeep` once per Base. The stage owns production,
population and both allocation/resource passes. Frontend advance and the ordinary
turn notification follow success. Canonical Bases never invoke legacy economy
posting. The explicit registered positional Pop roles and configured design-to-Def
resolver provide presentation; no display names or guessed attack statistics are
used.

Each Base commits atomically, but the complete turn is not a single transaction.
A failure after transition begins marks the world terminally failed and schedules
the existing admission failure cleanup. Mutations, retry and saving are blocked;
a successful prefix of Bases cannot be posted again. Unsupported size-one Base
removal and later-turn mineral deficit are surfaced by the real upkeep stage.
AI, multiplayer and their controller sequencing remain outside this local policy.

The actual claimed-Start fixture now checks the initial +1 mineral and matching
baseline, zero arid nutrient posting, and a configured productive tile's +2 food
(rainy Farm yield capped at two because its nutrient technology is unowned).
It continues through two queued end-turn requests, checking exactly one second
posting across both human owners and unchanged RNG pairs. A later Base with an
unsupported facility target tests committed-prefix failure, blocked save/retry and
real failure cleanup. A reversed cross-owner registration order verifies faction
grouping before saved Base order. These are synthetic source/map compositions using the real
native dispatcher and owned records. The cached Linux integration run passes
all 53 affected native groups with allocation tracking, including these cases.
The default local SMAC script now queues authenticated native first-Base and
selected Scout/Rover events. The actual definition and Base script handlers run
before creation; their existing canonical growth guard prevents legacy posting.
An explicit session-only eight-role binding chooses WORKER alongside the still
registered TALENT and positional specialists. Use validates the actual PopDefs;
saved allocation/Pop records remain authoritative. Manual clicks consume these
same configured roles, retaining their earlier explicit-fixture fallback.

New campaigns default to Librarian, the currently admitted first-Base difficulty.
The existing UI difficulty choice remains available and is never rewritten at
startup; unsupported selections fail the producer explicitly. This temporary
GLSMAC policy does not claim a general original difficulty initializer.

The two default-script Gaia/Sparta fixtures now load real bases.gls.js start/turn
handlers, create canonical allocation/knowledge, seal actual selected Units,
execute first movement and end-turn, check postings, and restore exact records.
Their synthetic source and generated-map policy fixture are explicit. Targeted
LLVM15 analysis passes for all six changed runtime translation units in 12.32s.
This checkpoint does not establish interactive rendering, a seven-player default
world, energy posting, or platform validation of this gameplay batch.
