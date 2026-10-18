# Saved human-faction membership

The original [setup and ending-input producer](behavior/ending-input-producers.md)
keeps human membership separate from active factions, current focus and transport
roles. GLSMAC now owns that authority in its simulation frame. Local human-only
calculations consult it, and a failed disposition produces a human-loss request
only for a human member. Eliminating or restarting a faction does not rewrite
the membership bits.

New-world admission explicitly marks the accepted human Player controllers.
Their identities remain the existing GLSMAC dense simulation IDs in accepted
controller order; the field does not infer profile ordinals or current focus.
The existing transport/ownership checks remain additional requirements. This
change does not admit AI calculations under human rules or create AI controllers.

Simulation V15 saves an unsigned-byte membership mask. Bits may name inactive
admitted factions, but never native faction0 or an identity outside the saved
roster. A human-loss request must name an inactive human member. V11–V14 port
frames migrate according to their historical all-human controller contract;
this is a port-format migration, not an original-save or arbitrary-role inference.

Tests cover masks independent of unchanged Host/Player roles and activity,
unknown/native/oversized masks, nonhuman loss requests, legacy migration, and
actual world save/Start preserving both an active nonhuman bit and an inactive
human bit without inventing an ending. Human-only owner queries reject both
cases without changing the world or either RNG. The non-dense transport restore
fixture remaps the independent membership authority with its explicit identity
remap rather than silently retaining old bit positions.

Remaining controller work includes explicit computer-controller admission and
its startup/action/economy paths, the disposition survivor/reference conditions,
and saved ending reason/pending/processed/exit state joined to presentation and
Continue. Current human-loss presentation remains an acknowledged text overlay;
original cinematic/presentation parity and the full conquest controller are not
complete.
