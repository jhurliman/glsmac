# Conventional movement integration — 2026-10-17

Integration source: `5bc62766a526037e0c63a50609063c1d53af2b2f`, combining movement source
`4fa1514d48a206e79d907f5d2b4d3f8812754399` with the selected Spartan Rover,
Rover presentation and resource-text changes already in `864ad416`.
This is the integration branch; pending slices remain separate from main.

| Check on the combined source | Result | Elapsed |
| --- | --- | --- |
| Native Windows Debug CTest | 8/8 | 90.55 s |
| Linux Debug CTest | 8/8 | 78.53 s |
| Linux focused allocation tracking | 18 groups passed | — |

The focused run covers conventional statistics, both selected-Spartan admission
and restoration groups, four movement groups, six Unit groups and real Scout
and Rover frontend/preview teardown. Full CTest also runs the Client matrices.
Independent peer review verified exact movement production deltas and retention
of all old and new tests. The fixture's single-player and selected-Rover flags
are separate, including the real Rover frontend caller.

The [movement source receipt](../conventional-empty-ground-movement.md) records
its full leak-enabled sanitizer run and 337-unit analysis with a final scoped
check after tightening the base fence. Those results cover its stated source;
this note does not relabel them as a new combined sanitizer or full analysis
run. The latest complete Windows/Linux/macOS checkpoint remains
[the d6afde1a receipt](unit-rover-resources-2026-10-17.md). Combined sanitizer,
macOS and full analysis are due at the next coherent gameplay checkpoint.

The real move event now commits Unit spending/failure bytes, tile membership
and paired gameplay RNG together. It handles the bounded ordinary, road, tube,
river and fungus cases, preserving continuation and the upkeep retry marker.
It remains restricted to an explicitly admitted local single-player world with
no registered bases. Friendly-HQ support and actual conventional startup are
separate follow-ups. No complete match or new interactive GPU validation is
claimed.

The private manifest is `out/conventional-movement-integration-manifest.json`,
SHA256 `1ff840eea036ac9384c4c62ff1ef503a028faa056759060668c9cb5f379c4e84`. It records binary and log hashes. Human research files and
original assets remain excluded from implementation commits.
