# Owned faction economy and Base energy query

Simulation V9 requires one owned economy record for every exact roster simulation
ID. It preserves psych/labs tenths, signed32 treasury and pending debit, fractional
research carry0..99, and directed treaty bits. The fixed SMEC/V1 record is112bytes;
outer simulation decoding bounds that frame before copying. Restore preserves
all fields and never replays new-world initialization. Pact remains the existing
separate knowledge authority.

Resource profile V3 adds raw RULES21 starting energy, frozen by Global14 and
World28. The producer clamps that rule to0..1000, then adds the last supported
faction ENERGY scalar with checked signed32 arithmetic. This scalar is outside
the eight-record faction list. The accepted STARTUP-ECONOMY001 prefixes establish
psych0/labs5, debit0, no treaties, installed treasury10/Morgan110 and separate
flat labs0. Research fraction0 additionally has a static producer: original
5c8b9a clears EBX and5c8d0e writes it to the submission fraction word. That field
was not separately poisoned in the seven-profile experiment. The strict port
RULES parser is not a claim of full original custom-text lexical equivalence.

`Game::GetCanonicalEconomy` returns a copy for the authenticated local human
owner. `GetCanonicalBaseEnergy` consumes the registered Base's actual final
worker/specialist allocation, orbital/tile intake, current EFFIC, completed owned
Headquarters, exact registered facility source IDs and current/baseline target.
It calculates waste, allocation, actual specialist outputs, multipliers, unequal
allocation penalties and baseline-qualified Stockpile. Source-backed free
facilities waive their maintenance; arbitrary display names do not. Direct flat
labs remain0 only for the fully validated supported faction keyword domain;
current social RESEARCH is applied later, never substituted for flat labs.

The query admits the saved no-project campaign, no Treaty commerce and at most
sixteen stored specialists. Unresolved facilities, project/orbital facilities,
Command Center special maintenance pricing and unsupported fees reject. Future
commerce and maintenance-deficit disposal need actual consumers. Arithmetic
rejects evaluated signed32 overflow; this is an explicit port domain restriction.

Private native posting stages current-RESEARCH scaling/fraction, selected
noncompleting research progress, credit and maintenance before scalar writes.
Punishment Sphere23 halves research first; Genejack22 does not. A zero increment
preserves progress/target as the actual research-add early return specifies.
Faction debit settlement subtracts once, clears the debit and floors credits0.
The turn controller owns scheduling; no saved posting cursor was introduced.

These private operations are deliberately not called by canonical upkeep yet.
Fresh campaigns have no chosen research target. Actual target choice/cost and
completion/Secrets processing must be connected before normal posting activation;
this change does not make ordinary first turns fail for missing targets. The
future caller must skip removed Bases, use surviving final allocations, post each
Base in the measured research-before-credit/fee order, then settle the faction.
Raw laboratory/gross-income statistics, ecology and full economy upkeep are not
claimed here.

Validation is focused. Four actual state/arithmetic helper groups pass, including
exact codec restore/rejection, four literal two-Base credit/debit sequences and
60 enabled original research increments/fractions from12 contexts. The separate
research-disabled wrapper bypass is excluded. The pinned research fixture is a
literal projection, not a modeled expected result. Four actual energy helper
groups pass27 waste,29 allocation and32 tail rows, plus the adjacent original
none/Skunkworks10/Hologram11 psych controls. The latter corrected an earlier
projector ID mapping; no other facility IDs were shifted.

Seven changed production TUs pass focused LLVM15 analysis; the corrected pure
energy TU also passes. Changed C++ and three new live Base groups are syntax
checked. Those live groups cover actual source startup, current versus retained
ratings, registered free/paid facilities, private posting rejection and world
restore into absent current state. Their linked execution is pending the shared
cached integration build. No full suite, platform matrix, original executable
rerun or default posting activation is claimed.
