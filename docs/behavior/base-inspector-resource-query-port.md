# Owning inspector resource calculation

The ordinary active local-human `select_base` wrapper now queues
`open_smac_base_inspector` through the existing native worker-command dispatcher.
The queued owner authenticates the Base, prepares the existing ordinary worker
and resource calculation, and publishes its updated cache and Pop projection.
Base data is sent before the frontend selection opens its screen. Multiple queued
opens retain the final requested selection independently of Base hash order.

Opening does not post nutrients, minerals, credits or a turn, and does not draw
from gameplay RNG. The UI's existing resource-preview getters remain read-only;
repainting an open screen does not enqueue another calculation. Continuation
screens, foreign/retired viewers, network sessions and legacy noncanonical Bases
keep their existing separate paths. This uses the current admitted local-human
worker producer; it does not supply missing AI/network allocation or projects.

The [original repair-boundary controls](post-flood-worker-repair-boundaries.md)
establish that opening the inspector invalidates its resource-cache key before
querying, while a matching cached query retains stale allocation. The native
port test opens two ordinary Bases through the actual Game wrapper and event
queue. It checks no immediate mutation, fresh caches, data-before-selection,
last-request ordering and unchanged stores, credits, RNG and turn. Existing
manual worker/recalculation groups, two GSE navigation/transferred-view scripts,
targeted correctness analysis and focused sanitizer checks pass.

This connects the ordinary opening boundary. The complete climate transaction
still needs its Base/Dome/status/deletion and old-surface Unit effects. This does not enable that live climate transaction. No new title-screen or rendered acceptance is claimed.
