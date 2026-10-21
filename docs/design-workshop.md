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
