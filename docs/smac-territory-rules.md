# Supplied-topology territory core

`game::backend::map::smac_territory::Rebuild` returns complete owned land/sea
claims from supplied parity-grid terrain/region IDs, positive border distance,
and explicit current Base record order. Later records win equal-distance ties.
Land and sea use their measured distance/region rules, with the sea foreign-land
veto. Previous claims are not inputs. This API does not create regions, infer
record order from IDs, publish live claims or initialize campaign knowledge.

This focused publication extracts source
`3e3608d65d9f6c4dd4d0043d19993c2303271b54` onto CI-policy prerequisite #168
(`4fd801516878e1971549fadfeb2c61e9e2cf4cd5`). The core function and its input API
are unchanged. `ReplaceClaims`, its knowledge dependency and its two adapter
test groups are deferred to their actual dependency stack. The positive-border
admission here is the source contract; later frozen-rule integration is separate.

The retained native helpers compare all 14 original complete maps (14,336
claims) and check strict cutoff, ties through the original maps, wrapping,
region identity, sea half-cutoff, signed32 border endpoint and 15 invalid inputs.
Both exact helper entrypoints passed when compiled with the actual production
TU using Clang15, C++17, `-O2 -DNDEBUG`. Targeted production-TU analysis also
passed with no diagnostics. No new whole-engine build, full suite, sanitizer
or platform run is claimed; the draft PR's selected Linux CI remains separate.

The retained fixture is byte-identical to source, SHA256
`23ff44ed2bf88caaee26e5b704018645e3dd2e7f98f509c5b1ed9bdd764fc25f`.
Its pinned original provenance and unused worker-mask metadata remain intact,
but this publication does **not** validate those 12 worker frames. The exact
`Rebuild` function text (LF, stripped surrounding whitespace) has SHA256
`e6e217e2085a7399cec6eca8dd0f00e44a2b9eabe9c2b1738511803ade1d006a`.
