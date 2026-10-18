# Ecology bloom admission and selection

`AdmitSmacEcologyBloom` consumes one raw CRT draw before testing the final stored
damage threshold and saved native-life/editor gates, including damage0. Active
perihelion halves the threshold with signed integer truncation. The exact gate
order and masks follow the [original handler receipt](ecology-handler-research-cache.md).

`SelectSmacEcologyBloom` queries actual semantic terrain inputs in the original
48-offset order. It preserves the first24/outer24 distinction, diagonal fungus
weights, removable-improvement and first-eight bonuses, sea rounding, one-step
horizontal wrapping, vertical clipping and strict score comparison. A zero
score never selects a winner. Outer candidates use the original distance
formula and later-record nearest-Base tie rule. The real-Base query flag means
the original Base item is present and its affiliation nibble is0..7.

The [sparse terrain supplement](ecology-selector-terrain.md) supplies independent
raw inputs, rather than precomputed weights. Its five original-executable
controls reproduce the winner coordinate, ordinal and score, every CRT draw
and resulting CRT state, with the shared random state unchanged. The projected
Planet neighborhood selects offset17 at63/19, score39, after eleven candidate
draws. Native flat/cylinder corner and later-Base tie fixtures also match.
Additional port controls visit all48 literal offsets, distinguish score ties
and zero results, improvement/sea weighting and deep versus surface fungus.
Eighteen original handler controls cover threshold and suppression admission.

Both functions are detached building blocks. Their caller must stage the CRT
stream together with history/terrain changes before publication. They are not
called by Inspector queries, and this change does not yet connect them to live
upkeep. Landmark protection belongs after selection and after the H increment;
it must not influence the chosen tile. Terrain mutation, native creation,
saved one-shot status and warming/climate consumers remain subsequent work.

Validation: cached incremental build;48 focused native groups and three GSE
inspector scripts pass in the aggregate/selection integration checkout.
Targeted LLVM correctness analysis passes SmacEcology.cpp and its two known
production consumers, BaseEcology.cpp and GameCanonicalTurn.cpp. These receipts
cover pinned stock SMAC; equivalent SMAX execution remains to be verified.
