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
Native offscreen receipts are now documented in
[the first path](behavior/design-workshop-first-path.md) and
[the preview/parent supplement](behavior/design-workshop-preview-parent.md).
They cover 43 picker admissions, four commit calls, edited model composition,
constructor geometry and fonts. Full-frame boot/compositing remains unverified.
Ask the user only for specific inaccessible screen states if local capture
cannot obtain them; screenshots alone cannot establish costs or mutation rules.

## First editor rules implemented

`SmacWorkshop` provides component-card source identities and effective technology
admission, retaining unavailable cards and omitting explicitly disabled rows.
This is the technology predicate, not general advanced-component compatibility.

The first commit API operates on `SmacStartupDesignState`: Scout tuple reuse,
an Infantry/Hand Weapons/Synthmetal/Fission/no-ability draft, last-slot allocation
in an empty faction bank, automatic or supplied names, and exact-name repeated
commit reuse. It uses the shared original Fission builder and installed-language
name producer. Unavailable or unsupported edits throw before publication.
Occupied-bank replacement, reactivation and existing-custom renaming remain
outside this API's admission boundary until their bookkeeping/prompt policy is
connected. A separate mutable Workshop design store was not introduced.

The named-design flag survives codec V3; prior V1/V2 bytes remain unchanged for
states that do not require that flag. Existing simulation serialization embeds
the same registry.

Base production now opens the first Workshop draft and shielding picker. The
popup uses owned `deswin.pcx` with indexed transparency, the controlled native
1024x768 constructor position, component rectangles, clipped captions and the
observed Arial sizes/colors. Early Fission stats, automatic names and ordinary
Workshop mineral prices come from the live frozen catalog and shared design /
mineral-factor producers. Workshop cost excludes the separate Base prototype
surcharge. English headings currently match the purchased reference.

Apply and Done use the existing game command queue, authenticate the local
Base/owner, and recheck design state plus owned technologies before publishing.
UI request IDs correlate acknowledgements; they do not authorize edits. Cancel
discards the pending draft and retains earlier successful Apply operations.
An older Done reply cannot close a newer session on the same Base. Enter commits
and closes only on success; Escape dismisses the picker or draft.

This remains the bounded Infantry / Hand Weapons / No Armor-or-Synthmetal /
Fission / no-ability path, with automatic naming in the UI. Native Rename,
existing-custom editing, browsing/categories and other component controls are
still required. The main CVR preview is implemented; small component previews
and animation remain absent. Saved Workshop designs
are not yet offered by the production chooser; prototype completion and
runtime armored-unit identity remain required for the first end-to-end playable
receipt. The current popup overlays Base management; the original full parent /
console composition still needs its complete implementation and live reference.
This is not a pixel-match or complete Workshop claim.

Validation for the first live entry: cached incremental WSL build; eight focused
live command/production/Rename groups; six affected GSE UI scripts including
duplicate/stale replies, cancel/reopen, unavailable cards and Enter/Escape;
targeted correctness analysis on three production translation units; focused
ASan/UBSan on six translation units with the eight native groups. A real hidden
Linux 1024x768 run opened the editor and picker, selected No Armor, applied Scout,
accepted Done with Enter, reopened and cancelled. Original owned assets were
loaded normally; no backend gameplay commands were injected by the capture
harness. That visual run used software graphics and no audio, so it makes no
hardware performance or audio claim.

### First-prototype production pricing

`Game::GetSmacProductionCost` now quotes saved unprototyped Workshop designs
using the shared ability-free component basis, stored owner difficulty, frozen
FREEPROTO presence and the actual queried Base's Skunkworks. The Workshop's
ordinary displayed cost remains separate. Source RULES 15..17 retain land/sea/air
percentages in a V2 production profile; V1 profiles retain absence and exact
bytes, and do not silently acquire stock defaults. This does not yet admit
armored designs to the production chooser or implement prototype completion.

Validation: 927 independent original executable price receipts with owned
component data; 26 focused native groups (including three complete-screen
cases), six affected GSE scripts, targeted correctness analysis of ten production
translation units, and the same 26 groups with eight relevant translation units
instrumented under ASan/UBSan. See [the native pricing contract](behavior/design-workshop-first-prototype.md).

### Main Workshop model preview

The live shielding selection renders the assembled Infantry / Fission model,
with Synthmetal inserted when selected. It uses decoded user-owned CVR files,
per-part shade tables and the initialized software palette. The original
Workshop camera and geometry-bounds centering are separate from map/card
presentation; the shared indexed rasterizer remains the same. The 256-square
internal result is blitted at native client center (295,164), without stretching
it to the enclosing 311-square panel. The UI currently presents yaw zero;
rotation and component-card models still need their native timing/views.

All four complete original indexed/depth frames match (Scout and Synthmetal,
yaw 0 and approximately 0.05), along with 14 independent synthetic bounds
controls. These are decoded-asset-to-frame tests, with frozen native outputs as
expectations rather than runtime inputs. See [the renderer contract](behavior/design-workshop-renderer.md)
and [the exact bounds store schedule](behavior/design-workshop-bounds.md).

Validation: cached incremental WSL build; 31 relevant native renderer groups
with owned assets; six affected GSE scripts; five production translation units
under targeted C++ analysis; the same 31 groups with five relevant translation
units instrumented under ASan/UBSan. Actual hidden Linux normal and focused
sanitizer runs displayed Scout, opened the armor picker, and closed cleanly.
This software/no-audio capture does not establish hardware or audio performance.
The existing first-design commit restrictions still disable some otherwise
known-technology armor edits in occupied startup banks; full custom editing,
production admission, other models and the original parent console remain.
