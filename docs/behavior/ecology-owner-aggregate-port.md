# Live owner ecology aggregate

The canonical owner pass now clears its own retained ecology aggregate before
Base traversal, including an owner with no Bases. Other faction words remain
unchanged. Resuming a research question continues the existing pass; it does
not clear the aggregate again. This follows the original
[owner/Base sequence](base-ecology-research-aggregate.md) and
[empty-owner and post-research cache controls](ecology-handler-research-cache.md).

After each Base's research finishes, and before credits and maintenance, the
consumer adds `trunc(4 * cachedDamage / clamp(4 - currentPLANET, 1, 99))`.
It reads the committed upkeep resource pass. An ordinary technology award does
not force a new damage calculation. Accumulation checks signed32 bounds before
publishing. Worlds lacking the ecology calculation authorities retain their
historical words rather than inventing newly calculated zeroes.
This includes admitted source presets of doubled width4 or6: their aliased
radius is outside current ecology calculation coverage, so a real Base turn
continues without clearing history or requiring a nonexistent damage value.

The live two-Base/two-faction test suspends twice at real research questions.
It checks contribution ordering, preservation of peer words, clearing an empty
owner, and clearing again on the following turn. Actual technology acquisition
raises the stored score by two: one control retains damage0 although a forced
fresh calculation would give1; another posts20 per Base, totaling40. Saves at
the research pauses serialize successfully. Existing continuation and history
tests cover restore; this new fixture does not itself resume those saved files.

Validation: cached incremental build;34 focused native groups and three GSE
inspector scripts pass. Targeted LLVM correctness analysis passes the changed
production translation unit, `GameCanonicalTurn.cpp`.

The original handler next consumes one CRT draw, even at damage0, then performs
threshold/gate checks, ordered bloom selection, clean-mineral history updates,
terrain changes and native-life/climate effects. Those steps still require live
integration. This change posts the aggregate only; it does not claim a complete
ecology handler or change the RNG stream.
