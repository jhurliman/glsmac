# Faithful Design Workshop

The user requested the complete original unit customization screen on
2026-10-20. This is a functional and visual release requirement for both SMAC
and SMAX. A static screen or a list of decoded components does not satisfy it.

## Audited starting point

The live Base production dialog still displays `Workshop unavailable` in
`GLSMAC_data/default/ui/parts/game/popup/base_screen/buttons.gls.js`.
There is no Workshop popup implementation in that script directory.

Reusable code exists, with different admission boundaries:

- `UnitRulesTXTLoader` and `UnitDesignResolver` decode source components and
  resolve predefined records. They do not establish Workshop legality.
- `SmacPredefinedDesigns` and `SmacOpeningDesigns` construct verified Fission
  startup records and costs. General reactors, arbitrary naming, collisions,
  replacement and Workshop transactions remain outside their evidence.
- The CVR loaders render bounded Infantry Scout and Spartan Rover profiles.
  These are not a general Workshop preview renderer. Workshop camera, scale,
  lighting, attachment selection and animation need separate controls.
- The ordinary unit-production caller explicitly rejects first custom prototype
  completion. Creating an editor record alone would therefore leave the main
  gameplay loop incomplete; production admission must be closed in the same
  working slice.

The purchased installation contains `deswin.pcx`. Its existence does not prove
its crop coordinates, palette or final composition. `Script.txt` and
`xscript.txt` contain `#DESIGNSHOP`, `#DESIGNAUTO`, `#DESIGNLABELS`,
`#DESIGNNAME`, redesign warnings and component-picker captions. `tutor.txt`
describes the controls and their intended visible effects. Load these through
the selected installation/ruleset; do not redistribute original art or text.
Executable observations must establish the actual transitions and geometry.

## Implementation sequence

1. Capture a native SMAC Workshop at a known logical resolution with Scout
   Patrol selected. Record executable/assets, faction, technology state,
   design records and preview phase. Capture its component pickers separately.
   Establish original source rectangles, palette, font metrics, borders,
   selected/unselected controls and preview composition before implementing
   the layout. Retain raw captures privately and publish suitable comparison
   images with the visual PRs.
2. Implement one end-to-end early Fission design: open from the game and Base
   production, select an existing design or empty slot, choose legal components,
   preview stats/model/cost, name and Apply/Done. Cancel must discard the pending
   edit while respecting changes already applied, according to native behavior.
   Save/reload the accepted design, select it for Base production, complete its
   first prototype and place a playable unit with the same design identity.
3. Extend the shared construction/eligibility/preview path to all chassis,
   weapons and equipment, armor, reactors and legal special-ability combinations.
   Verify restrictions, technology unlocks, noncombat/PSI statistics, carrying
   capacity, prototype charges and automatic names against original controls.
   Check advanced reactor and two-ability states explicitly.
4. Complete design browsing, presets, rename, obsolete/reactivate, redesign,
   retirement and Upgrade All, including confirmation, cancellation, occupied
   design slots, affected Units, energy charges and Base queues. Persist the
   resulting state and route multiplayer changes through the authoritative
   command path. Verify original rules rather than inferring them from captions.
5. Close SMAX differences, original keyboard/mouse behavior, Help/Datalinks,
   screenshots and milestone native-platform validation. The optional guided
   tour remains excluded by the user's earlier instruction.

Each step should produce focused reviewable changes. Component records, design
identity, costs, legality and previews must be shared with gameplay consumers;
avoid a separate approximate Workshop simulation.

## Visual and behavioral acceptance

Compare at the original logical 4:3 resolution before presentation scaling.
Use original pixels as the baseline, recording any PRACX/display alterations.
Require matching artwork rectangles, text baselines and advances, colors,
selection states, control bounds and model silhouette/placement. Compare a
fixed preview phase; test animation timing separately rather than hiding the
entire model behind a large screenshot-diff mask. A single attractive default
screen does not close pickers, confirmations or advanced designs.

The minimum behavior receipt follows a newly named design from component
selection through Apply, close/reopen, save/reload, Base production, first
prototype completion and usable Unit. Cancellation and unavailable choices
must leave authoritative state unchanged except for effects the original
actually commits. Redesign/retirement/upgrade need explicit affected-Unit and
production-queue checks before admission.

Use cached incremental builds and affected native/GSE tests. Run targeted C++
analysis on changed production consumers; use focused sanitizers for preview
ownership, design allocation and persistence. Reserve full platform runs for
the working Workshop milestone, not individual layout adjustments.

## Reference work requested

The existing `Reverse engineer Terran.exe` task has been asked to prioritize
the first native Workshop capture, asset/layout provenance, picker eligibility,
preview composition and Apply/Done/Cancel/design-to-production transitions.
No Workshop-specific native receipt or pixel-match result is claimed yet.
Ask the user only for specific inaccessible screen states if local capture
cannot obtain them; screenshots alone cannot establish costs or mutation rules.
