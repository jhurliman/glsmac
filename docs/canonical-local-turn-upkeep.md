# Canonical local turn upkeep

The native local `complete_turn` request now binds the incoming turn number when
queued. Dispatch admits only the actual local Single controller and the admitted
Single/Player human roster, checks exact current+1, and processes Bases in saved
record order. Duplicate queued requests cannot run the same upkeep twice. Direct
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
baseline, zero arid nutrient posting, and a configured productive tile's +3 food.
It continues through two queued end-turn requests, checking exactly one second
posting across both human owners and unchanged RNG pairs. A later Base with an
unsupported facility target tests committed-prefix failure, blocked save/retry and
real failure cleanup. These are synthetic source/map compositions using the real
native dispatcher and owned records. Runtime validation is pending the shared
cached integration build; changed production and fixture TUs pass syntax checks.
Default-world canonical activation is the immediate follow-up, not claimed here.
