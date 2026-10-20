# Ordinary console exploration admission

Ordinary SMAC tile selection now uses saved local observer exploration for the
terrain card. Unknown selection removes the previous preview widget and detail
labels and shows only Unexplored and coordinates. Resource-view clicks cannot
read through that state. Selecting explored terrain creates the preview again.

The original [console/grid contract](behavior/known-grid-and-console-fog.md)
executes eight original console selection repaints: Unity Survey alone does not
admit a preview, elevation, moisture or features. Faction reveal does; another
observer's exploration or reveal does not. The128-file frozen evidence bundle
was verified against manifest SHA256
`9c36cbf7f85151152217cd54226497d09f2cc9f068684134b787cf002b6c317f`.
Its source font/layout sinks do not establish exact historical console pixels.

## Live callers

`Game::AllowsLocalTileInspection` authenticates the current local participant and
reads the existing saved knowledge on its owning thread. It handles ordinary map
dimensions; this reader is not limited to the retained80x80 Planet snapshot.
Missing or unready SMAC knowledge does not authorize inspection. The existing
non-SMAC presentation path remains outside this contract. Editor-mode translation
and foreign-object cards are still separate, unfinished boundaries.

Both tile selection and Base selection now include the knowledge flag. Tile
notifications capture coordinates and the current game-session identity, then
resolve the backend tile during delivery. They no longer borrow a frontend Tile,
a backend wrapper, or a stack-local pointer across the queued callback.

The native preview widget also checks its owned snapshot before considering the
legacy fallback. This prevents a stale image appearing while the script receives
updated knowledge. A new snapshot refreshes the current selected tile's text as
well as the preview image. The explicit revealed inspector retains its diagnostic
presentation; it is not ordinary game fog.

## Verification and limits

- Cached native build and13 affected native groups pass, including real backend
  knowledge reads, preview fallback guards and six deferred Base-selection cases.
- Both tile-preview GSE scripts pass. Unknown fixtures provide coordinates only,
  so accessing terrain or resources fails. Known/unknown/known transitions prove
  old widgets/details are removed and new known previews can be created.
- Five changed production translation units pass correctness analysis.
- Focused six-TU ASan/UBSan passes15 native groups, including callback rooting,
  reset/cancellation and deferred Base selection. Other objects are uninstrumented
  and leak detection is disabled.
- A real1024x768 ordinary-menu SMAC Planet startup accepted the first-base name
  and research prompt. Selecting unknown tile76,56 removed the previous tile
  preview and details. Its terrain card remained identical over a resource-view
  toggle (13,029 pixels in ROI140,571,241,700). Opening/closing the owned Base
  screen worked. A later unknown selection10,70 also stayed hidden. The process
  closed normally with exit0 and no tracked OpenGL leaks. Allocation tracking was
  not enabled. Known-return behavior is covered by the GSE transition test,
  rather than claimed from a second live known-tile capture.

Actual known selection before moving to an unknown tile:

![Live explored console card](https://raw.githubusercontent.com/jhurliman/glsmac/2c5b3264d7863e50891283219f8d1b4d20e9525e/console-fog/known.png)

Actual unknown selection, with the old card image and details removed:

![Live unexplored console card](https://raw.githubusercontent.com/jhurliman/glsmac/2c5b3264d7863e50891283219f8d1b4d20e9525e/console-fog/unknown.png)

The screenshots expose the remaining ordinary main-map terrain disclosure gap.
This change does not implement the main map's black/grid overlay, foreign entity
presentation, exact unknown-label typography/position, or complete console parity.
