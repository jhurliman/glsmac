# Gameplay map and functional console acceptance targets

User reference: `codex-clipboard-d2376193-89c4-47fc-9f27-b449ad1af097.jpg`,
supplied 2026-10-18, 1920x1080 JPEG. This defines two separate acceptance targets.
Neither target is presently declared complete.

## Reference geometry and comparison

The installed `console2.pcx` is 1024x257. Resizing the reference to 1024x768
puts its console at y=511, with the major asset panel boundaries at x=245 and
x=776. This strongly supports a 1024x768 original stretched to 1920x1080:
x scale 1.875, y scale 1.40625, relative horizontal stretch 4/3. The original
capture resolution is inferred, not established by JPEG metadata.

Preserve the supplied JPEG. Derive the comparison image directly from it in
one resampling step to 1024x768; do not repeatedly resize intermediate images.
Use this derivative for layout and appearance, not a claim of exact original
pixels. Obtain fresh lossless original-game captures for strict comparison.
The renderer uses a 4:3 viewport; larger or wider windows letterbox it and
map input coordinates through the same viewport transform.

Compare map and console independently. Register fixed console frame anchors
once; do not permit arbitrary local warping that could hide layout defects.
Use a small spatial/color tolerance for this JPEG's resampling artifacts,
with thresholds calibrated against an original capture when available.
Keep text content, badge values, panel bounds and input hit targets as exact
semantic checks. Separate static captures from timed animation sequences;
capture at controlled phases rather than hiding incorrect animation by masks.

## Target A: map rendering

- Match terrain projection, elevation seams, coastlines, water, forests,
  improvements, roads and visible grid against an identical saved scene.
- Match Base and unit art, palette/lighting, depth ordering and sprite anchors.
- Reproduce every visible unit order/status overlay, including the reference's
  `H` badges and adjacent health bars. Establish the original order-to-glyph
  mapping, colors and visibility rules from evidence before wiring it.
- Match single- and double-digit Base population badges, explicitly `10`,
  including font, box, padding, anchor, contrast and overlap ordering.
- Match Base names and selection/blink timing without frame-rate dependence.

Use a deterministic developed-map fixture with multiple Bases, population
9/8/10, multiple units/orders, terrain improvements and stacks. Rendering
coverage does not imply the complete gameplay rules that created that scene.
Prefer importing the corresponding original save if it becomes available;
otherwise document that this is a controlled equivalent scene.

## Target B: functional bottom console

- Original frame, panel subdivision, fonts, colors, spacing and button states.
- Selected Base/unit information: population, name, current production and
  progress; live tile preview, elevation, terrain, improvements and coordinates.
- Faction identity, leader, integrity and might; Faction Dominance graph using
  retained game history, not decorative invented curves.
- Stack strip: Base/unit icons, order badges, health and combat statistics;
  selection and navigation update the inspected object consistently.
- Political/terrain minimap modes, viewport indication, navigation and mode
  buttons; correct hidden information for the observing faction.
- Menu, Commlink, Turn Complete, M/I controls, year and energy bound to real
  state and commands. Document any unsupported action rather than silently
  accepting a click or presenting fabricated values.

## Execution order and validation

1. Finish the active gameplay continuation change without losing its WIP.
2. Establish the deterministic scene and capture harness; retain separate map
   and console crops, full screenshot and input/capture conditions.
3. Focused PR for population/order overlays and anchors, with comparison images.
4. Focused PRs for console frame/font/layout and connected selected-object data.
5. Connect faction/history graph, minimap and remaining command controls in
   independently reviewable slices; exercise each visible control.
6. Run the full scene comparison and interactive acceptance pass at a meaningful
   integration milestone, then promote the lossless original capture baseline.

Use cached native builds and affected tests during development. Reserve the
platform matrix for shared rendering/input changes and integration gates.
Each PR requires exact-head GitHub Codex review and relevant validation.
