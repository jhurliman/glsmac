# Retained ecology history

Simulation V25 owns three independent eight-slot signed32 arrays: clean-mineral
history H, major atrocities and the retained research/ecology aggregate. Save
and restore preserve every word, including slot0 and unoccupied factions,
without sampling RNG, recounting facilities or recalculating Base damage.
V11–24 keep this authority unavailable; missing history is not a zero-filled
record reconstructed from current gameplay.

The initialization contract follows the four original
[owner-prefix controls](ecology-input-lifetimes.md): turn-zero initialization
clears only that owner's H and atrocities, leaving aggregate and peer words
unchanged. Nonzero-turn initialization preserves every word. Signed diagnostic
values survive decoding; the existing ecology calculator separately admits its
measured arithmetic domain when a value is consumed.

The actual Game Reset path now retains the preceding world's optional ecology
history in engine storage. `PrepareNewSimulation` transfers it, then applies
fresh initialization only to admitted faction owners. Cold GLSMAC engine
storage explicitly starts at zero; this is port initialization policy, not a
claim that the original bulk-resets all arrays on every New Game. The native
[new-world prefix](new-world-ecology-word-boundary.md) preserves all24 words
until its first faction initializer. That receipt does not establish a complete
new-game return or initialization of every slot. Unoccupied retained slots and
the prior aggregate are therefore not cleared speculatively.

The live resource-pass connection still needs mutable world ecology globals,
per-Base damage and the actual owner-pass aggregate/CRT/event sequence. This
change does not claim to implement the later aggregate reset, clean-history
facility/event writers, atrocities or bloom behavior. It establishes the saved
authority and real reset/initialization callers those integrations require.

Validation: cached incremental build;44 focused native groups, including the
complete simulation-state test module, actual Reset/new simulation, claimed
immediate/queued Start, all six startup difficulties and actual campaign file
restore followed by a turn. New tests retain independent source words, check
every incomplete word prefix and signed32 bounds, migrate V24 without invented
history, and execute the four native owner-initialization cases. Four affected
production TUs pass targeted correctness analysis. Focused ASan/UBSan on
simulation serialization and GameSimulation passes four history/reset/campaign
groups; other objects reuse the normal build, with leak detection disabled.
