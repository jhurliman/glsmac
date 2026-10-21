# Faithful Design Workshop

The user requested the complete original unit customization screen on
2026-10-20. This is a functional and visual release requirement for both SMAC
and SMAX. A static screen or a list of decoded components does not satisfy it.

## Historical starting audit â€” 2026-10-20

This describes the pre-implementation baseline; the dated slices below track
the implemented editor and remaining acceptance.

At this baseline, the live Base production dialog displays `Workshop unavailable` in
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

## General conventional preview composition

The asset-to-frame renderer now reproduces all 15 full indexed color/depth
frames in the [general preview contract](behavior/design-workshop-general-visuals.md):
Infantry/Laser/Synthmetal/Fission, Infantry/Laser/Plasma/Fusion, and
Rover/Impact/Synthmetal/Fission, including all four separate cards for each.
The runtime derives the Infantry mount from AVGMTGUN.CVR's two frames, retains
per-asset direct versus attached transforms, and generates faction materials
from the actual faction PCX sample and Gamma100 palette. Bounds includes hidden
Impact objects and root VLIGHTS geometry; drawing skips hidden objects and uses
the selected detail level. Laser cards use the original clipped kernel and
Rover lights retain every nonterminal stream record.

The live popup uses the composer for its current draft, with faction art
supplied by the actual Base owner. Decoded compositions are cached per Surface
and draft/material identity. The broader tuples are exposed through the same
Surface API, but remain outside the current live component picker/commit slice.
This does not establish complete Workshop screen or all-unit parity.

Validation: all 15 new frames match native color, depth, kernel count and
processed-record count, from owned assets rather than captured runtime operands.
All 32 selected renderer groups, 28 Workshop groups and six UI scripts pass;
the 32 renderer groups also pass focused ASan/UBSan. Correctness analysis is
clean for seven production translation units, including affected consumers.
An actual hidden software-rendered session opened Workshop, selected Synthmetal,
used Apply/Done and returned to the Base screen before exiting cleanly.

## Retained retirement tuple history

The design registry now retains all eight optional native retirement-history
rows: 64 eight-byte component tuples plus the next-write cursor per owner.
Fresh admitted factions receive FF-filled rows and cursor zero. The explicit
clear helper leaves the cursor unchanged; writes use the old cursor and wrap
63 to zero; human lookup scans every slot without consulting the cursor.
These rules come from the [registry/history handoff](behavior/design-workshop-retained-registry-and-history.md).

The existing human riot-prototype caller now checks this retained history
before allocation, returning its native negative result without changing design
authority when a retired tuple is found. The caller passes actual saved human
membership; temporary recipient calculation permission does not make an AI
human. The measured AI exact-match path reuses its record without consulting
human history. General AI prototype allocation remains unfinished.
Legacy frames with unknown history
cannot silently treat it as empty. The selective V5 design codec preserves
all known rows, including nonparticipating owners and untouched slots/cursors;
absent history keeps the existing V1Ã¢â‚¬â€œV4 encoding. Nonzero faction restart and
actual Start restore preserve it. The bounded design frame now accommodates
all seven full custom banks rather than stopping at the old 16 KiB budget.

Validation: eight independent native clear/append/lookup controls match all
4096 history bytes and eight cursors. Tests cover fresh and absent history,
cursor/extent rejection, complete codec restore, full custom banks, actual riot
lookup, and actual nonzero restart/save/Start retention. All 71 selected native
groups pass, with targeted correctness analysis of seven production files and
focused ASan/UBSan. Workshop's live overwrite transaction still needs to invoke
the history writer alongside Unit retirement and queue surgery; raw retained
record fields and intrinsic overlays also remain to be connected.

## Shared intrinsic flags and exclusions

The registry now retains sparse shared overrides for intrinsic IDs 0..63:
the complete flags word and per-owner exclusion byte. Frozen source definitions
remain immutable. Switching to an excluded matched intrinsic sets flag 8 and
clears only the requesting owner's exclusion bit. An already-selected match
leaves both unchanged; switching to an already-included match adds no flag.
The selective V6 design codec preserves every override and leaves V1Ã¢â‚¬â€œV5
encoding unchanged when no override exists.

Workshop commits publish the shared overrides alongside custom-bank changes.
Effective intrinsic state now reaches conventional design resolution, production
eligibility, ordinary capability refresh and restart selection. Existing Units
remain usable when their design is obsolete; exclusion belongs to new production
availability rather than the existing Unit's mechanical component admission.

Validation: four independent native flag/exclusion projections match across all
64 shared records, with custom banks preserved. The actual queued Workshop
command reactivates Scout, restores production eligibility, preserves other
owners' bits and permits existing-Unit updates before and after reactivation.
Tests also cover full overlay save/restore, legacy framing, field bounds,
capabilities and restart selection. All 124 selected native groups pass across
the initial run and focused fixture reruns. Targeted analysis is clean for nine
production translation units; ten focused groups pass ASan/UBSan with changed
producers and relevant consumers instrumented.

The [fresh/inactive draft contract](behavior/design-workshop-fresh-draft.md)
now records the separate initialization/reselection paths for the forthcoming
controller. Remaining work includes retained custom-record fields, occupied-slot
retirement/queue surgery, browse/rename/confirmation controls and the complete
component editor. Shared intrinsic overlays are no longer a missing prerequisite.


### Stock composition families

The phase-zero composer now covers stock chassis 0..8, combat weapon-power
selection, Colony/Former/Transport/Supply/Probe/Artifact packages, armor shell
branches and four reactors. Its broad entry accepts the actual installed weapon
power/mode and armor strength; it does not infer those fields from component IDs.
The existing narrow live entry remains compatible. Probe Speeder's temporary
Infantry remap affects composition only. Mount displacements come from the four
owned two-frame marker assets, with binary32 stores.

All fourteen new complete native indexed/depth frames match, including draw and
processed-record counts. The previous fifteen general frames remain exact.
Weapon cards use .15 for combat modes, .06 for Former and .08 for the remaining
packages. Only Single kernels use the card clip; ordinary Square2 stays separate.
Hovertank's grouped detail chain now retains group headers and selects the LOD
per draw; bounds still use root geometry. Truncation and malformed continuation,
group and extent tests cover that reader extension.

Validation: 49 relevant native groups pass, including parser/LOD regressions;
the same groups pass ASan/UBSan with eight production translation units and five
test files instrumented. Targeted correctness analysis is clean for five
production files and affected consumers. See the source-backed
[stock composition contract](behavior/design-workshop-stock-composition-families.md).

This is renderer groundwork for the full editor. The live picker remains the
Scout/Synthmetal slice; broader selection/commit controls, ability decorations,
nonzero animation and complete screen matching remain unfinished. Special
Probability armor card translucency remains with the existing dedicated card
renderer rather than this phase-zero composition entry.


### Retained full design registry

New worlds with installed naming labels now retain all 512 native records,
including inactive slots and global/nonparticipating banks. Initialization calls
the verified installed loader and opening constructors, then their native
mark/select pass. Typed custom designs and intrinsic flag/exclusion views are
checked against retained storage. Legacy port frames without raw authority stay
absent; no restore invents a zero-filled replacement.

The selective V7 design codec stores every 52-byte record. Unchanged name tails,
category byte43, claim/marker bytes45/46, opaque byte47 and signed prerequisite
bytes50/51 survive typed edits. Explicit synchronization patches modeled fields
before validation/publication. Workshop allocation uses the retained pool;
Workshop reactivation, bounded riot construction and Base defection publish the
whole design authority rather than swapping only owner vectors.

Whole-pool output hashes match 28 original marked startup controls: seven stock
owner/faction slots at four difficulties, including real automatic names. Tests
cover exact inactive/global bytes, name tails, opaque fields, atomic invalid
imports, stale mirror rejection, framing failures and maximum448 custom slots
plus the complete registry/history within the existing wire budget. Live command,
riot and serialized-restore regressions pass. There are 127 distinct relevant
native groups passing across the initial and focused fixture runs; 28 focused
groups pass ASan/UBSan with eleven production and six test files instrumented.
Targeted correctness analysis is clean for nine production files/consumers.

This establishes record authority for occupied-slot editing; it does not yet
implement its retirement prompts/queue surgery or full component controls.
The [animation controller](behavior/design-workshop-animation-controller.md)
also now has 24 native controls for the next animation integration. Nonzero-frame
pixel matching remains separate from the already-exact static compositions.

CI follow-up: three additional fixtures (selected Rover admission, production presentation, and founding selection) now synchronize their explicitly authored records and names. All three focused reruns pass; production code is unchanged by this follow-up.


### Common retained commit preparation

The common detached commit now combines tuple reuse, the original two-stage
allocator, cached Unit/production counts, general record construction and
post-build naming. Existing own aliases bypass new-component technology gates.
Inactive own selections retain their raw named flag as the source gate. The
live Base command uses this path for saves needing no retirement. Missing
caches are left absent: aliases need no allocation counts, and a wholly inactive,
included bank makes the provisional allocation independent of both count rows.
Other allocation paths require retained count authority.

Pending overwrite confirmation returns the original saved state. An accepted
preparation records the old eight-byte tuple, deactivates that record before
the replacement's prototype-coverage scan (`595d99`), and identifies the raw
design ID whose world retirement the caller must stage. Queue-only silent
in-place edits do not invoke retirement/history. Live Unit removal, global Base
queue surgery and the overwrite modal remain the next transaction integration;
the current command never publishes an unperformed retirement.

Ordinary saved-name copies now write text plus NUL while preserving the tail.
The fresh riot prototype constructor separately clears all32 name bytes, while
preserving its destination opaque byte/prerequisite. The seven complete native
saved pools match, including Infantry/Rover, Fusion, abilities and long names.
Tests distinguish pending/accepted retirement, prototype-coverage ordering,
queue-only silent edits, aliases lacking technology, short rename tails and
fresh-constructor name clearing.

Validation: 25 selected native groups pass, including the three regression
fixtures found by registry CI. The22 Workshop/registry/codec groups also pass
ASan/UBSan with six production and three test files instrumented. Targeted
correctness analysis is clean for five production files/consumers; changed
producer/caller follow-ups reused unchanged successful checks. No UI source or
pixels changed in this slice.


### Confirmed live design retirement

The live Workshop command now accepts an explicit overwrite answer. An unanswered
request returns the original saved state and the old design name/cached local
Unit count. Declining Done keeps the edited draft; declining Apply restores the
retained armor/name supported by the current narrow editor. Enter/Escape decline,
and the modal preserves the original request fence and Apply/Done intent.

Accepted retirement stages all global raw-design Unit matches in descending
physical order and all registered Base queues without an owner filter. Matching
current production becomes Stockpile; matching pending entries are removed
without promotion. Minerals, production baselines, Hurry state and retained
production-reference caches are preserved. Registry, occupancy, knowledge,
ordinary Unit counts, queue refreshes, design history and replacement publish
before despawn callbacks. Failed preparation and replayed acceptance leave the
world unchanged.

Validation:26 selected native groups and six UI scripts pass. The23 relevant
Workshop/registry/codec groups pass focused ASan/UBSan with nine production and
three test files instrumented. Targeted correctness analysis is clean for six
production files/consumers. A normal software-rendered layout preview exposed
text overflow; the modal now measures and wraps its paragraph into clipped lines.
The preview injects an example prompt into isolated copied UI data; live retirement
is verified by backend fixtures, not claimed as an interactive full-bank capture.

The full-bank fixture covers foreign-owned users of the same raw design ID,
multiple pending references, unchanged source caches, a late locked-tile failure,
coherent post-commit callbacks, stale replay and serialized world restoration.
The full component picker, saved-design browser, original modal frame/font
matching and complete screen pixel parity remain unfinished. The port also lacks
the original separate raw Faction aggregate total-Unit field; this slice updates
its actual roster and ordinary per-design counts rather than claiming that raw
buffer parity.


### Complete stock shielding path

The live shielding editor now admits all ten SMAC armor source IDs when their
technologies are available. Its read-only quote uses the shared retained
constructor instead of the obsolete first-design-only path, so a partially
occupied bank no longer hides otherwise valid researched armor. A pending
replacement is previewed on detached records without retiring Units or consuming
history. The existing live commit owns explicit overwrite consent.

The model surface consumes installed weapon power/mode and armor strength as
part of its cache identity. It no longer infers armor strength from source ID.
The side card admits the same stock armor IDs as the picker. Probability Sheath
blends its shade operands against the actual623x368 indexed deswin backdrop;
a real render caught the incorrect flat-background blend before commit.

Validation:27 selected native groups and six UI scripts pass. The live shielding
test preserves the owned stock defense metadata while explicitly remapping
prerequisites into its synthetic technology namespace. It saves armor1..9
through actual queued commands in increasingly occupied banks, verifies quotes,
checks unchanged non-design world state and restores the whole saved world.
Together with the existing basic path, every stock armor ID is exercised.
Thirty relevant native groups pass focused ASan/UBSan with fifteen production
and four test files instrumented; all34 component frames,29 general/stock model
frames and the complete indexed backdrop checksum remain verified. Targeted
correctness analysis is clean for eight production files/consumers.

An isolated software-rendered UI harness revealed all ten cards and cycled every
selection under sanitizers, then verified the corrected Probability Sheath card.
This harness overrides card availability only in copied UI data; backend legality
and save/restore are separate live fixtures. Its preview is not a full-bank
interactive save or native whole-screen pixel comparison. Other component
pickers, normal-open/browse/reselect, PSI text formatting, general production
admission and full Workshop screen parity remain required.


### PSI armor display text

The live shielding picker, side-card Level line and central stats now use `Psi`
for negative armor strength; the side card reads `Level Psi` without a colon.
This follows the [original text controls](behavior/design-workshop-strength-text.md)
for -128,-2,-1,0 and12 on all three armor surfaces. Positive strengths remain
ordinary decimal and are not multiplied by reactor power. These strings share
the current editor's installed-English presentation boundary; general localized
UI label loading remains future work rather than being claimed by this change.

Validation: the actual Workshop GSE script checks all five signed-byte controls,
UI label data for the PSI picker card, and `Level Psi` / `1-Psi-1` after
selection. All six affected UI scripts pass. C++ source is unchanged from the
shielding slice, so its successful build/analysis/sanitizer evidence is reused.
Weapon/noncombat formatting and original full-screen glyph/layout matching
remain open. The completed interactive-editing and normal-open handoffs are
retained for the next selection/component implementation; their references do
not themselves implement the live UI.


### Apply uses the native argument0 reselect path

Apply now reselects after a completed commit attempt, including failed edits
and declined overwrite, before its acknowledgement reaches the live editor.
Pending overwrite consent and stale commands do not reselect. Done closes only
on success and retains the edited draft on failure.

The shared detached selector follows4490b0(id,0): active records copy stored
components and the complete ability mask, rebuild the first two ascending
selector bits without compatibility cleanup, and clear only raw flag80.
Named custom text equal to its current automatic name is normalized by clearing
named bit2 and the draft string; other named text is retained. Intrinsics give
an empty draft name. New/inactive requests resolve to the lowest inactive own
slot, reset basic components/reactor/abilities and retain the current draft name
and all raw flags. Exhaustion falls back to the owner's first record. Foreign
bank input and oversized draft text are rejected before publication.

The live narrow editor consumes restored armor/name before reporting a failed
Apply. Bookkeeping can therefore change the original selection bit even when
component construction fails; failed preparation still cannot publish tentative
designs, Unit removal or queue changes. Actual removal notifications are sent
only after a removal transaction committed. Morgan's marked startup regression
now verifies exactly the expected80-bit clear and no other world mutation.

Validation:29 selected native groups and six affected UI scripts pass. The new
live failed-Apply/Done fixture covers saved records after technology loss and
New fallback/name retention. The detached selector checks raw pools, opaque
fields, named/automatic text, three-bit mask retention, intrinsic PSI rejection,
full-bank fallback and owner7 selection. Twenty-six relevant groups pass focused
ASan/UBSan with nine production and three test files instrumented. Correctness
analysis is clean for seven production files/consumers.

This is the reselect contract used by the current Apply caller, not completed
design browsing. Normal opening passes argument1 and still needs the general
recommendation controller. The full component/UI projection, browser strip,
Rename, compatibility refresh and full-screen pixel acceptance remain open.
No new rendering or native-platform matrix was needed for this state transition.


### Native ordered browse data reaches the live query

The owned Base Workshop projection now exposes the original5bdb60 ordered raw
IDs and active/excluded/flag/stored-name metadata. Included active designs come
first; all64 own slots remain represented, with inactive records ordered by ID
before obsolete designs. Excluded own records with a nonzero retained Unit
count precede excluded own records with zero; excluded intrinsics come last.
Intrinsics require activity, effective prerequisite admission and nonnegative
weapon power. Custom records do not gain a new component-technology filter,
and foreign banks never participate.

The query reads retained counts rather than recounting live Units. Missing
counts stay undefined. If an excluded active own record needs missing count
authority, the browse projection itself is undefined; independent included/New
order can still be returned without inventing a row of zeros. Inactive stored
names are not exposed as a New caption or editable draft. Raw IDs/flags survive,
and no selection bookkeeping or prototype allocation runs in this read path.

Validation:31 selected native groups pass. The added policy fixture covers all
five ordering classes, unsigned count255, disabled/PSI intrinsic admission,
unknown custom technologies, missing authority and foreign-bank exclusion.
The live query fixture checks deliberately stale cached count7, inactive flag82
with poisoned stored text, exact read-only world state and absent count handling.
Twenty-eight relevant groups pass focused ASan/UBSan with nine production and
three test files instrumented. Targeted analysis is clean for four production
files/consumers. The unchanged UI script evidence from the preceding reselect
slice is reused; no rendering or platform matrix was repeated.

This supplies the ordered data for the browser; its visible strip, scrolling,
card hit/selection routing, full draft projection and normal-open controller
are still required. The prior41 original browse-list controls are reference
evidence, not claimed as41 new C++ whole-list replays by this change.


### Conventional human recommendation model

The detached post-capability-refresh controller now ports the original597de0
producer and597390 preview policy. It generates equipment upgrades and the
human land, sea, air, missile and worker role families from installed rules,
current ownership, explicit faction inputs and retained design/count state.
Each proposal is applied immediately: alias unexclusion can admit a later own
row during the same prefix. Mode3 locks the published tuple/category without
suppressing later same-category alias side effects. Full banks preserve the
incoming draft when no alias or replacement candidate exists.

The input context deliberately requires every scalar authority. Missing rank,
AI policy, social Industry, capabilities or retained air-summary counters is
an error; it does not become zero. The installed MORALE bonus is named
`innate_morale`, correcting the reference's earlier `personality` label. Rank,
Base count and the five current AI priorities are separate authorities; see
[the semantic input map](behavior/design-workshop-input-authority.md).

Validation:32 selected native groups pass. The new C++ fixture replays22
original controls, all414 proposal tuples, each published tuple/mode/filter/
return and complete512-record final pool digests. Inputs are declared registry,
counts and technology state; scalar projections come from independent refresh
of declared input Faction buffers, never captured output tuples. Caller state,
selector/name and retirement history remain intact. Twenty-nine relevant groups
pass focused ASan/UBSan with nine production and three test files instrumented;
targeted correctness analysis is clean for four production translation units.
No rendering changed, so unchanged UI evidence is reused.

This model is not yet the live normal-open caller. Its host adapter still needs
current typed rank/AI policy and retained air-summary authorities, full component
projection and the argument1 selector composition. Unsupported PSI custom
strategic evaluation, modified tables, AI callers and alternate/project policy
remain explicit boundaries. The existing narrow live editor still opens Scout;
this receipt does not claim that behavior is fixed. The separately completed
[saved-strip contract](behavior/design-workshop-saved-strip.md) supplies the
next visible browser geometry, captions, navigation and raw-ID selection path;
original thumbnail model pixels and full-parent screen acceptance remain open.


### Complete draft transport and live preview

The live Workshop command now accepts chassis, weapon, armor, reactor, the full
24-bit ability mask and name, together with selected raw ID, state fence,
request correlation and optional overwrite answer. The four newly transported
fields form an all-or-nothing group. Legacy armor-only commands retain their
old basic tuple; malformed groups, out-of-range values before narrowing, foreign
banks and invalid names are rejected before queueing.

`get_smac_workshop()` retains its opening query. Its seven-argument form accepts
selected ID, chassis, weapon, armor, reactor, abilities and name to quote that
complete draft. Read-only quotes and Apply acknowledgements carry a `draft`
object and matching component metadata. Armor alternatives are priced/named
against the complete supplied tuple. Aliases bypass new-component admission;
a new tuple requires available, compatible abilities and legal capacity. Unknown
retained counts remain unknown whenever the allocation decision needs them.

Apply restores the complete saved tuple after success or failure, rather than
only armor/name. Failed Done retains the edited tuple. The actual UI consumes
that response, preserves all fields in later submissions, and passes the restored
chassis/weapon/reactor to its main preview. Existing correlated-response and
overwrite fencing remain in place.

Validation:33 selected native groups and all six affected GSE UI scripts pass.
A new live synthetic-catalog control quotes/saves an armed Speeder tuple with a
nondefault reactor and ability, verifies exact world state outside the design
bank, exercises the actual wrapped query, rejects partial/invalid/foreign input,
checks failed Apply and aliases after technology loss, and restores a whole-world
save. These are live adapter controls, not new original-executable observations;
previous independent construction/commit/reselection evidence remains applicable.
Thirty relevant groups pass focused ASan/UBSan with ten production and three
test files instrumented, including the Base wrapper. Correctness analysis is
clean for seven production files and affected consumers.

The visible picker set is still armor-only and ordinary opening still uses the
basic tuple. General side-card art, compatibility/suggestion transitions,
saved-strip selection, broader naming and the normal-open host adapter remain
required. This transport change does not claim pixel acceptance or completed
ability rendering. No new screenshot or full platform matrix was appropriate
for this protocol change; component/browser presentation follows separately.


### General domain and PSI automatic naming

The shared original594320 naming path now covers land, sea, air, missile and
PSI components. It preserves interceptor precedence, signed strength gates and
parity, domain-specific elite thresholds, sea utility wording, missile reactor
prefixes, inactive-name collisions and retained selected plan/chassis/reactor
inputs. Naming a draft does not require the selected retained record to match
the proposed tuple or be active. Commit allocation, stored-name truncation and
exact-tuple aliases keep their existing ownership.

The twelve native commit comparisons include both-PSI Infantry reusing intrinsic
Mind Worms with its stored name and unchanged complete pool. A generated name
is not substituted for an existing alias. Negative weapon strength skips compact,
defensive and interceptor wording; negative armor does not. A controlled -2
strength case verifies signed parity rather than an absolute-value shortcut.
See [domain naming](behavior/design-workshop-domain-naming.md) and
[PSI naming](behavior/design-workshop-psi-naming.md).

The installed Sea label is now frozen with the other naming text. Predefined
profile V3 transports eight labels; retained V2 profiles preserve their original
seven labels and byte encoding with Sea explicitly unknown. V1 remains unchanged.
A legacy profile does not acquire current English text during restore; branches
that need its missing Sea label reject rather than invent it. Translation/equality,
late host replacement, truncation and transport-budget tests cover the extension.

Validation:42 selected native groups pass, including all68 original naming
controls, twelve complete native commit-pool digests, signed override controls,
retained/inactive selection, collisions, and V1/V2/V3 profile transport. Thirty-nine
relevant groups pass focused ASan/UBSan with thirteen production and five test
files instrumented. Correctness analysis is clean for ten production files and
affected consumers; the changed namer and live quote consumer were reanalyzed
after the PSI extension, reusing the other unchanged results. The six unchanged
UI script checks from the complete-draft transport slice remain applicable.

This removes the land-only naming blocker for the general draft pipeline, not
the remaining screen work. The human recommendation producer still has its
separate PSI-custom strategic-evaluation boundary; naming support does not close
that gap. Browser selection, all component/ability pickers, suggestions, general
side-card art, normal-open host inputs and whole-screen pixel acceptance remain
required. The separately frozen [strip raster](behavior/design-workshop-strip-raster.md)
provides the next thumbnail implementation evidence; this PR does not claim new
model pixels or repeat a platform matrix.


### Saved-strip ordinary renderer

`CvrScoutRenderer::LoadStripDesign` now shares the owned-asset component recipe
while retaining the ordinary idle scene used by the saved-design strip. It does
not shrink the main Workshop camera. Stored chassis/weapon IDs select the entry
scale before the Probe body remap; the exact predicate is retained in
[ordinary actor scale](behavior/ordinary-actor-scale-predicate.md).

`RenderStrip` produces separate indexed body/depth and shadow planes at facing2,
zoom-10, phase0. Component offsets, direct attachment frames, grouped detail
selection and frame80 exclusion apply to both paths. The shadow kernel now
accepts counted streams as well as explicitly terminal-delimited streams.
`CvrStripImage::Composite` validates source/target/clip/alias bounds before any
target mutation, applies destination-index shadow lookup first, then keys body
index9. It retains caller origin and half-open clipping, including negative
origins; there is no alpha approximation or RGB shadow blend.

Four complete original bodies, depth planes and shadow masks match from owned
assets: Scout, Rover, armed Foil and armed Needlejet. Eight complete indexed
target copies match the two explicitly supplied native lookup precision states.
The existing owned-palette shadow producer matches the declared027f table; only
entry6 differs from037f. This does not establish the precision state reached by
the original full parent initializer. See the [raster handoff](behavior/design-workshop-strip-raster.md).

This is the renderer connection for the next live browser slice. It does not yet
display/select saved cards, initialize the original whole parent, render ability
decorations or native creatures, or establish other faction/animation parity.
The PSI reopen handoff is now available separately for the human producer.

Validation:39 selected native groups pass, including the four complete native
plane sets, eight copied targets, existing main/component previews and prior
Scout shadow receipts. Fifty-two renderer/decoder/frame/texture groups pass
focused ASan/UBSan with eleven production and five test translation units
instrumented. Targeted correctness analysis is clean for seven changed or
affected production units. No gameplay/UI script source changed in this slice;
no platform matrix or new whole-screen acceptance is claimed.

### Live saved-design browser

The Workshop now displays the retained browse projection in the original
console-local strip rectangle, with ten70-pixel columns, cyclic one-pass
scrolling and raw-design-ID click routing. The ordinary body/shadow renderer
is connected to real saved rows. Inactive rows expose no stale components or
name, and short lists do not repeat. A bounded16-image cache retains completed
indexed thumbnails rather than decoded model trees; hover/selection repaint
reuses those images and the installed palette/label/frame sources.

The original parent frame is now captured and independently replayed as a
whole indexed buffer. Its new handoff supplies installed Empty Slot/Obsolete
labels, six arrow sprites, generic card/frame styles and caller-specific
centering. The implementation uses the existing interface.pcx frame painter;
all four70x50 native frame rasters match. Native scanlines, transparent corners,
normal/hover/selected precedence and installed label text replace temporary
flat fills. See [parent capture](behavior/design-workshop-native-parent.md) and
[strip art](behavior/design-workshop-strip-art-and-centering.md).

Card selection uses a fenced `select_smac_workshop` game event through the
ordinary and canonical Colony Screen dispatchers. The original argument-zero
reselector retains saved component/name semantics after technology loss,
normalizes source selection flags, and chooses the first inactive own slot
with a basic tuple while retaining the unsaved name. Full-paint compatibility
and Neural capacity cleanup applies to the editable draft, not the saved
ability mask. Scroll arrows are read-only; card clicks preserve offset.
Opening and Apply retain their separate recenter behavior. Stale selection
responses refresh quotes for the current complete draft.

Chassis, weapon and reactor side previews now consume the selected draft;
shielding retains its existing indexed/translucent compositor and shared
centering sequence. The stored tuple is carried into Apply/Done as before.

Validation:37 selected native groups and two owned-asset frame groups pass,
including the queued card event, technology-loss selection, inactive fallback,
stale/foreign fences and complete non-design world-state preservation. Six UI
scripts pass; browser checks cover native columns, short lists, centering,
the left count sentinel, right-after-sentinel behavior and pending request
guards. Thirty-six relevant groups pass ASan/UBSan with sixteen production and
five test units instrumented. Correctness analysis is clean for seven
production units/consumers; Surface was rechecked after the final compositor
adjustment. An actual instrumented SDL/OpenGL run opened the strip, selected
Morgan's saved Synthmetal Garrison, scrolled and selected an inactive slot.
The final capture repeats saved selection with the corrected component anchors;
it exits0 with no sanitizer or tracked GL leaks. Software graphics/no audio.

This is a functional saved browser, not completed Workshop parity. General
picker controls, ability decorations, normal-open recommendation inputs,
remaining action controls/console panels, full stat/cost presentation and
timer-driven preview animation remain required. The raw component stat line
is not yet the complete original5932d0 formatter. The reference uses a blank
map fixture; live captures retain the current Base Screen behind Workshop.

### Accepted picker transitions and live query boundary

The editable draft and its two ordered ability slots now have source-backed
component-change callers. Chassis/domain changes, missile payload coercion,
weapon rejection, automatic suggestions and subsequent compatibility cleanup
follow the original order, including same-value returns after modal cancellation.
Manual ability selection preserves reverse order and empty slots; missile and
missing-Neural entry guards leave the draft unchanged. Automatic suggestions
are an explicit preference input, separate from owner technology admission.
See [interactive editing](behavior/design-workshop-interactive-editing.md).

The owned Base exposes `get_smac_workshop_edit` as a read-only query. It derives
technology admission from the live owner, carries the complete draft/name and
ordered slots, rejects foreign banks and validates wide script integers before
narrowing. Unavailable component choices retain the old choice before running
the original accepted caller; this query never commits or allocates a design.
The visible picker buttons still need to call this adapter. Preference
persistence, the other component grids and manual ability dialogs remain open.

Validation:35 selected native groups pass in both the cached build and focused
ASan/UBSan run (five production and two test translation units instrumented).
Controls compare15 complete native draft transitions, two ability-entry guards
and three modal returns. A real wrapped Base call verifies reactor admission,
manual slot gaps, wide-integer rejection and complete unchanged world state.
Correctness analysis passes on the two changed production files and three
affected consumers. LLVM15 crashed in `bugprone-sizeof-expression` for
UnitManager; that consumer passes with only that check disabled. No UI source
changed, so the previous six UI-script successes are reused. This is not new
whole-screen or platform-matrix acceptance.

### Native stat formatting and main placement

The main preview and saved browser now share the original5932d0 stat grammar.
PSI uses question marks, noncombat equipment uses zero, abilities decorate
attack/defense in source order, sea/artillery attacks retain parentheses, and
aircraft movement applies transport/SAM/Antigrav/Slow adjustments before the
1..99 clamp. Advanced reactors append their rating. Main captions additionally
append installed ability abbreviations in ascending bit order, including the
original trailing space for no abilities. Saved rows read their own retained
mask and reactor rather than the currently editable tuple.

The main stat baseline moves from canvas302 to270. The source layout rectangle
is y256..272, but the clip remains the whole preview rectangle; clipping the
glyphs to that layout line would cut descenders. See the additive
[presentation handoff](behavior/design-workshop-main-presentation.md), its
[native scalar receipts](behavior/design-workshop-main-presentation-cases.json)
and [preference/picker contracts](behavior/design-workshop-preference-and-picker-layout.md).
The cost warnings, cached-factor lifetime, component/ability controls and timer
contracts in that handoff remain follow-up work, not acceptance claims here.

Validation:24 native stat strings and four main captions match;36 selected
native groups and six UI scripts pass. The live browse projection checks a
saved reactor/ability-mask combination and unchanged complete world state.
The36 groups also pass focused ASan/UBSan with five production and two test
translation units instrumented. Correctness analysis passes on three
production files/consumers. A fresh instrumented SDL/OpenGL capture opens the
Workshop and selects the saved Synthmetal Garrison, showing the corrected
baseline; it exits0 with no sanitizer or tracked GL leaks. Software graphics,
no audio. An earlier capture remained in the production dialog and is excluded;
the successful retry waits for the button to settle and checks the Workshop
backdrop. No whole-screen pixel-parity or platform release claim is made.

### Live chassis/reactor pickers and accepted shielding edits

Chassis and reactor side cards now open their original5x4 component grids.
The live owner determines configured and known choices: unknown configured
definitions remain as unavailable silhouettes without labels, while disabled
definitions are omitted. Chassis uses its raw speed and source triad; reactors
display ratings1..4. The native frame painter, clipping, scale/yaw and shared
centering sequence replace whole-unit thumbnail substitutes. Installed TRIAD
and LABELS supply domain, Power, Shielding and Psi captions. The picker stores
at most16 completed images and their centering carry, not decoded model trees.

Chassis, shielding and reactor acceptance now call the live read-only edit
adapter, refresh the complete editable tuple/quotes, and retain the browse
offset. Escape returns the old choice through its accepted caller, including
the original cleanup/suggestion behavior. Automatic suggestions use the source
fresh-store enabled default and an explicit controller boolean; a preference
screen, persistence and original INI import remain unfinished. Ability slots
survive refresh and their names follow the draft mask. Missile shielding offers
only No Armor. Save/Apply remains the existing fenced queued transaction.

The new chassis path also exposes legitimate empty component scenes. The
[weapon side-card trace](behavior/design-workshop-weapon-side-card-empty-scenes.md)
establishes that Missile payload cards deliberately have no model. Empty parts
now retain transparent color and untouched depth/centering instead of throwing.
The ordinary Probe card requests independent ptmod.cvr; whole-unit Probe
composition and the explicit cached-body context retain their separate policy.
Hand Weapons keeps its direct scene ahead of chassis handling.

Validation:45 selected native groups and six UI scripts pass. Forty-five groups
also pass focused ASan/UBSan with seven production and three test translation
units instrumented. The existing full native model/component/strip receipts
remain unchanged; reactor picker internal planes match the existing native
component scene. Additional checks cover intentional empty parts, independent
Probe admission, configured-vs-disabled rows and the Missile shielding guard.
Correctness analysis passes on three production files/consumers, with Surface
and the renderer rechecked after the empty-scene correction. An instrumented
SDL/OpenGL session opens/cancels chassis selection, opens/accepts reactor
selection, selects shielding and applies the design, exiting0 without sanitizer
or tracked GL leaks. The observer resumed the same running process after a
cold-load timeout; incomplete frames were excluded. Software graphics/no audio;
this capture is not a performance measurement. The final renderer-only change
retains the captured ordinary scenes, as checked by the native frame receipts.

The weapon picker, manual ability dialog/decorations, preference persistence,
remaining actions, cost-warning presentation and timer-driven preview still
need their live implementations. This is not completed Workshop or whole-cell
pixel parity for every chassis/reactor definition.

### Live manual ability chooser

Both Special Ability cards now open the owner-backed text-list modal. Entry
checks second-slot technology before the Missile prohibition and displays the
installed SCRIPT warning with the loaded technology name. None is always first
and initially selected; unknown/incompatible abilities and the other slot's
duplicate are omitted. The menu query leaves the complete world unchanged.
Names/descriptions and cost descriptors come from the installed rules, with
raw weapon/armor cost bytes and raw chassis speed rather than strength, reactor
movement or faction mineral prices. Cost/Attack/Defense/Speed/None/OK/Cancel
and domain tokens use their installed localization identities.

Rows retain the native16px frame,19px stride, Arial Narrow16 bold baseline14
and clipped left/right spans. Selected/hover/ordinary frame styles29/28/27
have the original precedence. The recorded1024x768 console profile uses a600px
drawable list,12 visible rows, source-derived short-list height and300px outer
minimum. Extra minimum-height space stays below the buttons. Main frame,
caption, list border and inner console rails follow the source rectangles;
the scrollbar remains outside the narrowed main frame. See the
[chooser contract](behavior/design-workshop-manual-ability-chooser.md).

Hover/click and keyboard navigation remain local until OK/Enter; clicking a
row twice does not commit it. Escape/Cancel does not call the edit adapter.
Accepted None/ability updates only its requested slot and rebuilds the mask
without sorting, compaction or automatic suggestions. Arrow keys clamp and
keep selection visible; printable keys cycle by first character, Left/Right
leave the one-column selection unchanged, and Tab traverses the modal controls.
Visible selection repaint reuses row objects; scrolling alone rebuilds them.

Validation:37 selected native groups and six UI scripts pass, including three
native admitted-value lists and nine cost fragments. Wrapped Base queries
verify configured owner admission, duplicate exclusion, foreign-bank rejection
and unchanged full world snapshots. UI controls cover None-first short/long
layouts, selection/hover without mutation, cancellation, accepted slot gaps and
reverse order, scrolling, first-character matching and the second-slot warning.
The37 groups pass focused ASan/UBSan with nine production and two test units
instrumented. Correctness analysis passes for four production units/consumers.
An instrumented SDL/OpenGL run opens the real starting owner's None-only menu,
accepts None, displays the actual Neural Grafting requirement, dismisses it and
closes with Done; exit0, no sanitizer or tracked GL leaks. Software/no audio.

This does not establish full chooser pixel equality: the17-row layout receipt
is conditioned on supplied backing boundaries, and live overflow scrollbar
chrome/repeat timing and keyboard focus decoration still need fidelity work.
The remaining weapon picker, parent ability-card decoration, other actions,
preferences, cost warnings and preview animation remain on the Workshop path.

### Standalone Retire and retained inactive references

Retire now has a live illustrated confirmation and a fenced queued transaction.
It uses the selected stored design and cached count, then removes all raw-ID
Unit matches. Custom retirement updates history and Base queues; intrinsic
retirement preserves the shared design and every queue. Existing inactive
queue/baseline references and the retired starting-Scout provenance survive
save/reload, while inactive designs stay out of new production choices.

The original flags-zero modal returns its selected row on Escape as well as
Enter/Shift+Enter; double-click also accepts. The live SDL bridge supplies the
double-click event after ordinary mouse-down selection. The original
illustration, full caption frame and initialized OK sprite/glyph pixels match
their native surface regions exactly (53,152 compared pixels; the button has
four transparent corners). Normal, hover and pressed states are captured in
the real instrumented dialog. See the
[Retire integration evidence](behavior/design-workshop-explicit-upgrade-and-retire.md).
Upgrade, automatic family controls, persistent preferences, animated preview
and complete screen parity remain open; this does not complete the Workshop.

### Stored-design Upgrade action

`SmacWorkshopUpgrade.h` owns the original action7 candidate scan,504eb0 quote
and detached design flags of504cc0. The queued `upgrade_smac_workshop`
transaction connects these rules to the local human Base caller and suspended
Colony Screen dispatch. It enumerates retained targets, checks affordability,
supports decline and zero-charge bypass, and stages Unit states, owned queues,
refreshes, target reselection and treasury before publication. Existing Unit
transfer publication owns wrapping count deltas; production-reference caches,
queue order/minerals/retool baselines and retirement history are preserved.
The ordinary no-project campaign capability explicitly owns Nano Factory
absence; missing project authority is not treated as false. No new save format
is introduced.

Twenty-five compact original dispatcher inputs/results verify candidates and
per-unit quotes; four original scalars verify prototype/Nano rounding. Thirteen
actual-world scenarios cover custom/intrinsic sources, cached-zero and stale
counts, no own Units, prototype surcharge, High Morale gain/loss, multiple
targets, insufficient funds, late-animation rollback, duplicate queues, foreign
records and all four reactor health limits. Each resulting world saves/restores.

The live identity binding now resolves retained positive Projectile/Energy
ground combat weapons, positive armor, reactor1..4 and the passive High Morale
bit. Another owner's physical Unit can resolve its globally indexed inactive
source after Upgrade. These identities use existing conventional stat rules;
this does not remove the separate narrow tactical combat, Speeder provenance,
native/PSI, other ability or component-rendering restrictions.

The original chooser, confirmation, insufficient-funds, no-target and zero-cache
completion templates load from installed SCRIPT text. Single-pass substitution
preserves plural forms, word boundaries and emphasis. Stored labels include
localized Psi/Sea/Air, reactor suffixes and ascending ability abbreviations;
[eleven stat strings and three prompt pairs](behavior/design-workshop-upgrade-strings.md)
are checked against native receipts. The chooser appends the original unprototyped
` *` marker to the right stats field under its exact source/target flag condition.
Unsaved draft components do not enter this stored-source action.

The [long-list contract](behavior/design-workshop-upgrade-long-list.md) controls
column-major layout, native horizontal-key direction, literal-pipe measurement,
height and placement. Actual SDL harnesses render all13/63 synthetic rows and
select the last row. Measured maximum widths are166/174; resulting client sizes
are757x300 and1177x382. The latter reproduces native off-screen overflow. These
copied-data harnesses do not assert an all-technology campaign or full grid
pixel parity.

Validation:46 affected native groups passed for the transaction and consumers;
six focused groups and the UI script pass ASan/UBSan with19 production and two
test translation units instrumented. Production/consumer correctness analysis
is clean. The instrumented actual-game capture accepts the live Scout-to-
Synthmetal upgrade through selected-row Escape, reselects its target and exits0.
All79,352 compared pixels match the owned original:24,000 illustration,16,056
caption,13,096 opaque initialized OK pixels and26,200 choice-row pixels with
glyphs. Four OK corners are transparent. The body contains different live
names/count/credits and is excluded from this pixel claim. This software,
no-audio run makes no hardware-performance or audio claim.

### Original occupied-design replacement confirmation

The temporary two-button English overwrite popup is replaced with the original
[OVERWRITEPROTO modal](behavior/design-workshop-redesign-confirmation.md).
Installed text supplies the stored replacement name and cached unit-count byte;
prototype-name tokens substitute once, preserving dollar signs inside names.
The always-land illustration,755x300 frame, Arial Narrow body, two native choice
rows and initialized OK button share the existing modal rendering contracts.
Flags0 means Escape accepts the selected row; default row0 declines, while
selected row1 accepts. Existing queued request fencing and Apply/Done semantics
remain authoritative. The original literal count1 wording is preserved.

Five selected native groups and three Workshop scripts pass. Six UI production
translation units pass targeted correctness analysis. Five native groups and
the UI script pass with six production and one test translation units under
ASan/UBSan. An instrumented copied-data presentation harness reproduces all
three original body strings at baselines50/70/90 and accepts selected-row Escape,
then exits0. Its supplied name/count do not claim a live full-bank campaign;
the existing occupied-bank native fixture covers the actual transaction.
All53,152 compared illustration/caption/opaque OK pixels match existing original
surfaces. Body/choice glyph pixels are not part of that regional pixel gate.
The intrinsic informational-dialog follow-up is documented below.

### Intrinsic design informational dialogs

The live commit reply now carries typed INTRINSICTYPE/INTRINSICNAME notices
and the retained intrinsic name. Failed full-bank intrinsic edits preserve the
world and the original Apply reselection/Done draft behavior. Normal active,
tech-admitted matches include the selected intrinsic itself: a differing name
warns but succeeds, while an empty/equal name succeeds without a warning. The
return-zero unchanged branch is reached only after the whole alias scan fails;
it can warn about the name or return silently. The corrected
[branch contract](behavior/design-workshop-redesign-confirmation.md#selected-intrinsic-name-clarification)
and [native selected-Scout receipt](behavior/design-workshop-intrinsic-name-branch-cases.json)
keep those paths separate.

Both installed informational templates reuse the illustrated modal with no
choice rows and one OK button. A successful Done waits for acknowledgment before
closing the Workshop; rejected Done remains open. Acknowledgment sends no new
commit, and modifiers/duplicate acknowledgments/teardown are fenced.

Validation: six affected native groups and three Workshop scripts pass under a
1 MiB stack limit. Seven native groups and the UI script pass ASan/UBSan with
eight production and two test translation units instrumented. Eight production
translation units pass targeted correctness analysis. An instrumented actual
SDL session opens Rename, types a differing name, accepts Done, displays the
original-name warning and closes to the Base screen only after Escape; exit0,
no tracked GL leaks. All53,152 compared illustration/caption/opaque OK pixels
match the existing original surfaces; no full informational-body pixel parity
or full-bank desktop capture is claimed. No save-format change is introduced.


### Ordinary Workshop main-button fidelity

All seven ordinary actions share the native Arial-14 bold monochrome caption,
baseline15, installed palette foreground65 and 105x20 indexed frames. Apply,
Done and Cancel now read LABELS919/271/103, including their original uppercase
presentation. Hover and press both select frame29; normal uses frame27.
Caption placement follows the original integer advance calculation, with a
three-pixel inset and99-pixel available width, instead of rounding a half-pixel
center. This stays local to Workshop main controls.

The instrumented live session opens Workshop from the real starting base,
previews hover/press, applies the draft, selects a saved strip design and
cancels back to Base; clean exit0. All14,700 pixels across the seven normal
105x20 control regions equal the retained original full-parent capture. This
is button-region equality, not whole-Workshop parity. Three affected UI scripts
pass; targeted analysis covers six UI production consumers, with Surface alone
rechecked after the label fix. Focused ASan/UBSan covers the same six UI files
and the main Workshop script; unchanged backend checks are reused.

[Native button visibility and centering](behavior/design-workshop-button-visibility.md)
and [editor-only action contract](behavior/design-workshop-editor-prototype-reset.md)
separate ordinary controls from Scenario Editor PROTO/RETIRE ALL. Those editor
controls are not introduced into ordinary play. The conditional AUTO OFF
caption does not establish a reachable distinct ordinary action; no new effect
is inferred from its label. Main preview animation, recommendations and
remaining complete-screen presentation are still outstanding.


### Explicit nonzero model frames

The decoded Workshop renderer now accepts an explicit shared phase. Infantry
and Probe bodies, ordinary Infantry shielding and the Copter rotor use phase
modulo15. Speeder wheels and structural Infantry shielding pass the raw phase
to the asset frame selector, which retains its own frame count. Weapon and
reactor stages retain frame0. A visible Copter body increments the shared
counter after copying its rotor; a chassis-card paint and following main paint
therefore advance it separately. Hidden hierarchy inputs preserve the counter.
Selected frame bounds are recomputed before centering and projection; decoded
assets are reused. Rejected paints preserve the caller's phase.

Four complete color/depth frames match the new original-game
[nonzero receipts](behavior/design-workshop-nonzero-animation.md), covering two
consecutive yaw values and Copter entry phases14/15. The existing phase-zero
Workshop/strip/component checks remain green. Fifteen affected production
translation units pass targeted correctness analysis. Six production rendering
files and the renderer tests pass focused ASan/UBSan.

The model-composition layer is now connected to the live controller described
below. Negative signed phases remain outside the admitted raster domain; the
original overflow producer alone does not prove safe pixels.

### Live Workshop preview controller and cached sprites

Opening the Workshop resets its local yaw and paint count, paints its initial
component/main previews while hidden, then starts the original coalesced 40 ms
notification cadence. Each delivered notification repaints only the main
preview. Three-dimensional paints use the old binary32 yaw and then add 0.05f;
there is no artificial wrap at 2 pi or 64 radians. Selection refresh keeps the
clock epoch; close stops delivery and a fresh reopen resets local state.
Accepted Rename repaints the main preview once. The graphics-lifetime model
phase is separate and survives reopening. Visible Copter paints advance it;
the initial hidden compositions preserve it. The DOM exposes actual ancestor
visibility to the script instead of relying on a mock-only visibility method.

Cached previews follow the original signed icon and complete component/ability
tuple admission, with the Infantry weapon-icon fallback and selected-icon
precedence. The installed ordinary Scout has icon -1 and uses the rotating
model. Admitted cached sprites use their native trimmed source, 30/16 scaling,
311x311 clipping and destination-dependent indexed shadows. Ranked sprites use
the retained scratch plan, social ratings and admitted owned Probe technologies.
Decoded models and sprites are reused between notifications.

Evidence includes five complete original color/depth frames (including yaw65),
eight native rotation-helper cases, the complete 96,721-byte cached preview,
38 rank cases and 25 Probe technology cases. Both 256-entry shadow precision
profiles match their native receipts. The ordinary 53-significand runtime
profile remains the default; the explicit 64-significand harness profile does
not establish a different production initialization path.

Validation: 69 selected native groups and three affected UI scripts pass.
Focused ASan/UBSan instruments 27 production and three test translation units;
the same native groups and scripts pass. Targeted correctness analysis covers
43 production translation units and known consumers. The actual
instrumented SDL session verifies changing main-preview pixels, 192 timer
deliveries, no delivery after Cancel, fresh delivery after reopening, and a
clean exit with no tracked OpenGL leaks.

[Cached admission](behavior/design-workshop-cached-preview.md),
[blitting](behavior/design-workshop-cached-blit.md),
[rank and lifecycle](behavior/design-workshop-cached-rank-and-lifecycle.md),
[Probe rank](behavior/design-workshop-probe-rank.md),
[yaw range](behavior/design-workshop-yaw-range.md) and
[shadow precision](behavior/design-workshop-shadow-precision-lifetime.md)
record the original-game sources and private evidence hashes.

This does not establish whole-screen Workshop parity. Reserved scratch63's
cached-idle alias remains explicitly unsupported, ranked legacy states require
retained scratch metadata, and the admitted Probe profile requires explicit
no-project authority until project ownership is implemented. Special-ability
model artwork and remaining complete UI/gameplay behavior still need work.


### First custom prototype manufacturing

The ordinary ground manufacturing caller now stages the original first-completion
flag, human component-history propagation and newborn morale with its prepared
Unit. The existing prototype price is admitted before those flags change, so
this build keeps its original debit while later builds see the ordinary cost.
The first newborn receives one extra stored morale point, clamped to six; repeat
builds do not. Exempt charges do not replace the old prototype-bit predicate.

Component history reuses the constructor's source-derived weapon, armor and
same-domain chassis coverage. The completed record gets bit4; covered active
unprototyped own records retain their other bytes and gain word0x0104. Stored
inactive completion references do not acquire an invented active-record guard.
The detached full design state publishes with the newborn, and later riot
transfer composes from that state rather than overwriting it with an earlier
snapshot. A discarded factory or later rejected queue publishes neither flags
nor a Unit.

The six complete original design-pool returns in
[first-prototype evidence](behavior/design-workshop-first-prototype.md) match.
The live fixture saves a named design through the queued Workshop command,
checks detached/discarded preparation and a late queue failure, manufactures
first and repeat Units with the expected identity/home/morale and prices, then
restores the first-completion checkpoint and reproduces the uninterrupted
second-completion world. Existing definition-registry ordering is normalized;
remaining serialized world bytes are compared.

Validation covers59 selected Workshop/manufacturing groups and nine additional
consequence groups (riot/defection, abandonment and restart), with the enhanced
prototype/checkpoint test rerun separately. The same scope passes focused
ASan/UBSan with15 production and three test translation units instrumented.
Targeted correctness analysis covers35 production translation units and known
consumers; the history helper is rechecked after removing the unsupported
active-record guard. The unchanged UI/script and rendering results from#530
remain applicable. LLVM15's full-debug assembler crashes in one unchanged test
translation unit; line-table-only debug output avoids that compiler crash while
retaining sanitizer instrumentation.

This closes a manufacturing rejection, not the complete human-played Workshop
receipt. The simulation still admits its documented ordinary ground packages;
High Morale and other currently unsupported packages are not enabled by this
change. No original allocation-limit refund/aftermath is inferred: the captured
2048-Unit prefix already stores prototype flags and debits minerals before
allocation refusal, while the current port rejects at its factory guard.
Original full newborn production-state flags, complete UI gameplay acceptance,
remote replay and general SMAX manufacturing remain separate open checks.
