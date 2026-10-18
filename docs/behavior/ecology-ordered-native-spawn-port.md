# Ordered native spawn requests

[Six original stage controls](ecology-ordered-native-spawn-cases.json) establish
the post-terrain, post-admission ordered sweep and selected-square fallback.
PrepareSmacNativeSpawn produces an immutable request plan for designs8/9/10.
It does not call factories, publish live Units or advance RNG.

The sweep retains original nine-offset order, nearest-Base later-record ties
and actor-owner comparison. Positive cached ownership blocks only a real Unit
or Base item; native0 and invalid nibble15 stay distinct. Regional Base-count
bytes select designs. Non-design8 candidates increment the special counter even
when strict Base-count/history limits suppress an attempt. Sweep attempts count
factory requests, including failures, rather than successful births.

Fallback starts at sweep attempts, uses its separate selected-square design,
and is limited by signed current population and history/3. It deliberately
omits an ecology-owner tag write; only successful sweep factories receive the
actor tag. The plan contains failed-capacity requests too, so a live consumer
must not retry failures, drop attempts or condition later warming on births.

The planner accepts staged map/region/cache inputs and globally ordered Base
coordinates. It is entered only after the original AI, one-shot and H/Voice
gates. Those gates, actual allocation/stack/sight/counter changes, optional AI
assignment and later warming/climate remain the owning live callback's work.
This checkpoint is not a completed ecology callback. Signed32 multiplication
outside the measured arithmetic domain rejects before C++ overflow, following
the existing ecology helper policy; no native overflow equivalence is claimed.

Validation: cached incremental build and nine focused spawn/bloom groups pass.
The source-bound group compares every request and counter in all six original
streams, including full-capacity failures and tag0 fallback births. Additional
source-algorithm controls distinguish owner ties, stale cached nibbles, clipping,
signed population, maximum127 fallback requests and strict special limits.
Targeted correctness analysis passes SmacEcology.cpp. No platform or sanitizer
matrix was repeated for this detached producer; live ownership integration will
receive focused lifetime/bounds checks when connected.
