# Saved ecology event authorities

Simulation V28 retains raw faction status words and the global warming counter
needed by the live ecology event handler. These have separate availability and
lifetimes. Their authorities are the original
[status initialization and save controls](ecology-handler-research-cache.md)
and [warming save/reset supplement](ecology-selector-terrain.md).

Each of the eight faction slots owns an optional unsigned32 status word. Every
bit survives save/load, including the sign bit, unrelated bits and the one-shot
post-bloom hold0x4000. Fresh turn-zero owner initialization clears that owner's
entire word. Nonzero initialization preserves it; neither path fills unknown
peer words. Actual Game Reset retains the preceding world's available status
words until new-world owner setup. Cold GLSMAC engine storage explicitly starts
with zero words; this is port cold-storage policy, not an inferred whole-world
bulk clear in the original.

Warming is an independent optional signed32 global. Saved Start restores its
exact value without invoking the new-world reset. A fresh world establishes0
at the global reset boundary, independently of faction initialization, clean
history H and the per-owner ecology aggregate. It is not sea-level offset.

V11–V27 frames migrate with unknown status words and warming counter. V28 can
retain individual missing faction words while knowing warming, and vice versa.
Decoding rejects narrowing and incomplete suffixes without reconstructing event
history from terrain, facilities, population or current ecological damage.

Validation uses the cached native build:90 related native groups and four GSE
scripts pass across the save/startup, ecology and Governor integration runs.
Independent typed wire fixtures cover raw unsigned boundaries, signed warming
boundaries, per-slot absence, oldest/current legacy migration and every partial
event suffix. Actual Reset/new simulation preserves peer canaries and clears
warming. Actual saved Start restores warming17, owner status0x80004000 and peer
UINT32_MAX alongside the retained Base resource pass. Four affected production
TUs pass targeted correctness analysis. Seven focused native groups pass with
ASan/UBSan on Game reset, simulation initialization/codec and canonical snapshot
TUs; remaining objects reuse the normal build, with leak detection disabled.

This supplies saved authorities and their real lifecycle callers. It does not
yet set or consume the one-shot flag, advance warming during blooms, mutate
terrain, create native life or execute climate evolution. Those event stages
must preserve the established ordering when connected.
