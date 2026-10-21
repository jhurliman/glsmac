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
custom replacement/retirement, browsing/categories and other component controls are
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
The first-design commit supports existing-tuple reuse; occupied-bank allocation
and full custom editing,
production admission, other models and the original parent console remain.

### Existing design reuse and reactivation

Morgan's startup Synthmetal design is usable in the live Workshop even when
its legacy saved name is absent. Matching scans the active predefined designs
and then the owner's custom bank by component tuple. The selected record's
named flag controls draft-name transfer; target names and prototype state stay
independent of tuple identity. Switching to an excluded custom match clears the
owner's exclusion bit and retains original flag 8 along with all other auxiliary
word bits. Apply acknowledgements retain the selected design ID for the next edit.
Read-only legacy labels use the existing installed-language naming producer.

The registry emits V4 only when auxiliary flag bits are present. States with
all auxiliary bits zero retain their exact earlier V1/V2/V3 encoding. The active,
named and prototyped flags continue to have one authority. Standalone Rename,
unused-slot selection, intrinsic obsolescence overlays and full allocation /
retirement remain separate parts of [the complete native contract](behavior/design-workshop-custom-edit.md).

Validation: 19 focused native groups, including six original complete matching /
name-transfer cases with owned source catalogs and an actual Morgan startup
producer, reactivation and whole-world restore. All original faction, Base,
Unit and history buffers remain unchanged in those six reference cases. Six
GSE scripts pass, including selected-ID acknowledgement and stale/failure cases.
Targeted C++ analysis of five production consumers is clean; 25 selected native
groups pass with five relevant translation units instrumented under ASan/UBSan.
A live hidden Linux Morgan run selected Synthmetal, used Apply then Done, and
returned to the unchanged Scout Patrol queue. It exited cleanly. The capture
uses software graphics and no audio.

### Component models and armor chooser

Workshop side cards now compose the Infantry, standalone Hand Weapons, optional
Synthmetal, and Fission models at their original scales and clips. A painter
pass carries scene centering state in the original draw order. The reactor uses
2-by-2 voxel samples. Model previews and tint construction use the initialized
Gamma100 palette; existing map/faction rendering policies are unchanged.

The armor chooser uses INTERFACE.PCX small frames (styles0/2), original
105-by-91 cells, and fixed-yaw armor models. Unknown technologies retain their
silhouettes without labels. The four colored armor variants derive their shade
lookups from VBP.CVR and SYSTEM.TXT. Probability Sheath composites indexed
shades over the actual card backing. Empty page slots receive no card writes.
Technology visibility is separate from support for committing a draft.

See the [component contract](behavior/design-workshop-components.md) and
[picker backing contract](behavior/design-workshop-picker-backing.md).
Numeric expectations cover34 complete original indexed/depth frames, all four
6144-byte tint tables, the initialized palette and6144-byte translucent lookup.
These are model/primitive checks, not a complete-screen pixel-match claim.
Font masks/wrapping, the parent console, rotating previews, other chassis and
weapons, SMAX-only armor models, and full design editing remain unfinished.

Validation:32 focused native renderer/retained-owner groups and six GSE scripts
pass. Targeted analysis of five production translation units is clean. The32
native groups also pass with six relevant translation units instrumented under
ASan/UBSan. Normal and focused-sanitizer hidden Linux runs displayed the side
cards and armor chooser, selected Synthmetal, used Apply then Done, and returned
to the unchanged Scout Patrol queue. Both exited cleanly. These captures use
software graphics and no audio; no full platform matrix was repeated.

### General record builder and ability rules

The live first-design commit now calls the general original record builder,
using the retained builtin history for prototype coverage. The builder supports
all catalog reactor ratings and computes raw shared cost, transport capacity,
plan/category, owner claim and native prototype flags while preserving unrelated
record bytes. Coverage can come from different active builtin/custom designs;
reactors and abilities do not affect that scan. The original last-slot flag16
quirk is retained.

Ability compatibility and cleanup follow the separate native predicates. Cleanup
removes bits ascending, retaining the highest two compatible choices (one without
Neural Grafting), and preserves selectors when nothing was removed. Technology
admission remains the editor caller's responsibility. This does not yet expand
the live editor beyond Scout/Synthmetal: generalized naming, allocation/editing,
component models and picker interactions must be connected together next.
See [the conventional construction contract](behavior/design-workshop-general-construction.md).

Validation:40 complete original design pools,216 compatibility predicates and11
cleanup cases match. All26 focused Workshop/startup/custom-design groups and six
UI scripts pass. Targeted analysis covers the builder plus its Base and game
production consumers. The26 native groups also pass under focused ASan/UBSan
instrumentation of the changed builder and its test consumer. The screen and
save protocol retain the component pass's live validation.

### Installed-language automatic naming

Fresh SMAC sessions freeze seven Workshop words from LABELS.TXT with their
component catalog. The selective V2 unit-profile encoding retains those words;
absent labels retain exact legacy V1 bytes, including after settings restore.
Clients receive the host's words and do not reload their local installation.
Late replacements reject before publication, and a missing naming source leaves
host initialization retryable.

The conventional land naming producer combines the installed chassis, weapon,
armor, reactor and ability strings with these labels. It preserves native
parity and prefix rules, scans intrinsic and own-bank names even when inactive,
skips matching tuples and foreign banks, and handles localized Mk suffixes.
The untruncated producer includes the original Polic Rover quirk; saving retains
its separate 31-byte boundary. The live first-design commit uses this producer
when its frozen language fields are present and preserves the legacy path for
older sessions. Broader component editing and generalized allocation remain next.

Validation: all 40 independent automatic names match, alongside their complete
design-pool controls. 95 focused native groups cover host/profile transport,
legacy bytes, packets, world headers, construction and naming; six UI scripts
pass. All seven frozen naming fields reject late replacement in settings and
world headers. Targeted analysis of six production consumers is clean. 24
selected native groups pass with four production translation units and two test
consumers instrumented under ASan/UBSan. No platform matrix or unchanged UI
capture was repeated.

### Occupied-slot selection and overwrite decisions

The shared slot selector reproduces the native two-stage allocation scans:
changed intrinsic designs prefer the last inactive slot during the provisional
scan; an active candidate then runs the common scan, where the first inactive
slot wins. Fully unused excluded slots replace earlier candidates, while queued
same-role slots have their separate fallback rule. Active cached units trigger
the overwrite question only when no empty or fully unused alternative won.
Tuple matching precedes allocation and ignores stored names.

The live empty-bank commit now uses this selector instead of choosing an ID
arithmetically. Broader occupied-bank publication remains gated: the active-unit
cache is retained, but the separate production-reference cache and complete
retirement/controller transaction still need connection. The original cache
lifetime is documented in [production reference counts](behavior/production-design-reference-cache.md);
opening/applying Workshop must not insert a recount or rebuild stale saved rows.

Validation: 36 native allocation decisions match using their tuple, role,
flag, prerequisite and cached-count inputs. All 28 focused native groups and six
UI scripts pass. Targeted analysis of the selector and both production consumers
is clean; the 28 groups pass with the changed builder and test consumer under
focused ASan/UBSan. These checks establish allocation decisions, not completed
live retirement or occupied-bank UI behavior.

## Retained production-reference counts

The separate 512-byte owner production-reference cache is now saved with
economy state. New factions start with zero bytes. Normal owner summaries
rebuild current and pending references, including duplicates; queue edits and
advancement apply wrapping byte deltas between summaries. Workshop open and
Apply do not recount it. Older economy V4 frames retain absent cache authority
and their existing wire encoding; V5 carries the new row, including stale values.

The [Base lifecycle contract](behavior/production-reference-base-lifecycle.md)
is connected too: surviving capture and riot transfer debit the old queue
without crediting the recipient; razing and ordinary deletion retain stale
bytes; actual faction reinitialization clears the row. Founding remains a direct
uncounted initial assignment. Joined worker publications compose queue deltas
instead of replacing another Base's changes. Workshop command fences now include
both active-unit and production-reference cache presence and bytes.

Validation: all 13 independent native summary/manual controls match, including
duplicate entries, negative queue length, unused tail, foreign owners and byte
wraparound. The compact fixture projects negative-length manual loops as no
effective entries; the summary controls independently exercise that length guard.
The 158 relevant native groups pass across the original run and focused fixture
reruns, as do six affected UI scripts. Changed production translation units pass
targeted correctness analysis. Focused ASan/UBSan covers all changed production
files and the economy, live gameplay and simulation test consumers.

This supplies retained inputs for occupied-slot allocation. General occupied-bank
publication, retirement/history handling and the broader component editor remain
unfinished; the live editor still admits its existing Scout/Synthmetal slice.
