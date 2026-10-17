# Canonical research and suspended upkeep

Canonical upkeep now submits each surviving Base's final labs before its economy
credit and maintenance, then settles the faction once. A required manual research
choice suspends that exact native turn. The continuation owns stable owner/Base
IDs, its next stage and the already-calculated final energy result; it does not
repeat growth, production, lab submission or credit when the player answers.
Removed Bases are skipped. No saved posting cursor was added.

The existing claimed Start can pause during its first upkeep after the raw Units
have been sealed. Initialization still reaches frontend setup. Final turn hooks
and the active-turn notification wait for the answer. While suspended, only the
authenticated research event and session cleanup are accepted; snapshots reject.
Reset retires the continuation and request, while request IDs remain monotonic so
an old button cannot answer a later campaign. Commands queued before a pause are
discarded rather than applied against the resumed state.

`get_pending_smac_research()` returns null or an owned request containing its ID,
owner slot/name, bonus flag, and ordered technology ID/name/category choices.
`choose_research {request_id, technology_id}` validates the local caller and the
still-current offered pool. Retired/duplicate request IDs are harmless. The
`research_choice` hook announces both successor requests and the final clear;
the separate mandatory frontend chooser consumes this contract.

The cost query preserves the original cache early return, discovery-rate-zero
sentinel, each integer scaling boundary, actual map dimensions and intrinsic
RESEARCH sign. Its ordinary manual pool preserves category order, prerequisite
availability, first-offer exception and modulo exclusion. Evaluated signed32
overflow rejects explicitly. The live caller uses admitted local human owners,
their stored difficulty and the ordinary no-Stagnation campaign policy. The
validated faction keyword domain has no TECHCOST override; the measured default
constructor supplies100. AI, transfer and automated picker modes are not claimed.

Acquisition stages the owned technology record and any map revelation before
publication: ownership, score, Future count, discount consumption, target/cache
updates and the first-global Secrets marker. A completing submission resets labs
and processes at most one pending bonus before choosing the following target.
A zero increment returns without inventing a target or consuming Secrets. Map
revelation uses the existing saved Knowledge capability and observation producer,
not a second technology-side flag. Other acquisition side effects outside the
admitted ordinary campaign are not represented by this slice.

Technology V2 persists discount, deferred awards and pending Secrets exactly.
Simulation V10, Resource V4, Global15 and World29 reject older incompatible
records. New-world zeros and TECHCOST100 are grounded in
RE-SMAC-RESEARCH-START-INPUTS-001. RULES physical row8 uses the measured unsigned
hex/binary or decimal prefix reader and clamp0..1000; it does not broaden the
separate strict parser for other RULES fields. Restoration never reruns reset.

The self-authored fixture projects37 direct original cost queries,13 ordinary
manual pools and24 discovery-reader fragments, with complete input-file hashes
embedded in the generated header. The three actual native helper functions pass
when linked to optimized production sources. No expected arithmetic is generated
by the projector. Production and the two new actual claimed-Start tests pass
syntax checks; nine changed production TUs pass focused LLVM15 correctness
analysis. The latter cover initial pause/reset, duplicate answers, a real
second upkeep completing Secrets and revelation, once-only publication and
stable-world restoration; their linked execution awaits the shared cached build.
No full suite, platform matrix or original executable rerun is claimed here.
