# Restart map scale and landmark proximity

`ScaleFromGeometry` reproduces the native map initializer's integer
floor(sqrt((width/2)*height)). It counts parity-valid tiles, not coordinate
columns. Native80×32 custom geometry produces35, not its height32 or a
rounded36. Map load invokes this producer from dimensions; terrain/territory
changes preserve its result. The search reads its supplied resident scale
rather than guessing from a category or height.

`LandmarkBucket` follows the original225 ordered offsets in eight rings,
Manhattan distance0/2/4/.../14. It wraps x once when enabled, clips invalid
coordinates and returns the first qualifying ring0..7, otherwise32. Only raw
shared-detail bytes1..127 qualify. Detail0 and suppressed bytes128..255 do not.
Neither membership flags nor the registered landmark list supply this byte;
positive detail without membership/registration still qualifies.

[Frozen native cases](../src/gse/tests/fixtures/SmacRestartPreprocessing.h)
compare six scale results,23 scans and all2,074 actual valid word-read
coordinates. These cover registry-only/detail-only controls, same-ring ties,
suppression, every ring, wrapping and clipped poles/flat edges. Eight native
Q-gate controls feed the real search: Q6 fails at128 and passes at129; Q0 still
fails when the relaxing threshold reaches0. Three native resident-scale
discriminators check the custom distance6/7 boundary and an explicitly
unsupported dimension-only canary that preserves residentS35.

These are the decoded-input producers/helpers needed by the joined restart
caller. Shared-detail history still needs explicit fresh-map/save ownership;
absence of a registry or membership is not permission to invent zero detail.
This does not establish the original landmark/world generator or complete
setup/load/resize UI workflows.

Validation: cached Linux Debug build; nine affected native restart/package
groups pass with allocation tracking. LLVM15 correctness analysis of the new
production helper passes in0.46s; unchanged candidate-loop analysis is reused.
No ownership, serialization or platform changes warrant another matrix here.
