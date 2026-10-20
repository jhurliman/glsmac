# RE-SMAC-FIRSTBASE-SCHEDULE-001: intro, name prompt and first upkeep

Static caller-boundary extension of [FIRSTBASE result semantics](setup-modal-result-boundaries.md).
The pinned original's fresh local controller calls the introduction and its
FIRSTBASE prompt **before global advance, before first faction/Base upkeep,
and before normal human command entry**. This does not introduce a new
original execution: previous continuous controller receipts supplied the
entire introduction, then measured global/upkeep/command ordering.

## Exact fresh-local sequence

At controller `53be70`, EBP is zero and ESI is1. Introduction call
`53bedc -> 5c5b30(focusedFaction)` is admitted only when all hold:

1. Global turn `959fdc == 0`.
2. Resume cursor `9ba55c == 0`.
3. The focused faction `9430d4` belongs to human mask byte `959ff0`.
4. Mode global `944624 == 0`.

The nonlocal/multiplayer mode and nonhuman-focus paths skip this call. These
are source gates, not assertions that every possible scenario starts this way.

| Phase | Original boundary | Turn/state relevant to the prompt |
| --- | --- | --- |
| Starting world already exists | setup/founding has returned before `53be70` | Starting bases have generated names and starting production/resources |
| Introduction and landing visuals | `53bedc -> 5c5b30` | Turn0; select first live base owned by focused faction, or owned-unit fallback |
| FIRSTBASE | `5c621e` template init, `5c622c` modal | Existing-base path only; current live name is default |
| Commit name | `5c624d/54`, then `5c6256 -> 50cc80` | Edit the existing Base+13 and normalize exact duplicates |
| Finish introduction | redraw/poll/cleanup, `5c66cc` return to `53bee1` | No faction upkeep or global advance occurs in this continuation tail |
| Global advance | `53bf41 -> 53a530` | Existing measured fresh path advances turn0 to1, year2101 |
| Scheduled faction upkeep | `53caed -> 53bb60(owner)` | Social refresh, then per-base processing |
| Base upkeep | `53bb88 -> 53b7a0`; owned-base call `53b951 -> 50c760` | Resources, production and other ordinary upkeep now run |
| Turn-one numeric TECH quota | `53baf4 -> 5d5a70` after base loop | Conditional faction quota/selection, including University's separate first-upkeep award |
| Human command phase | `53cb4a -> 527eb0` | After that faction's upkeep; this is not the earlier intro/name phase |

The preceding first-command receipt's seven controlled worlds measured the
post-intro global/upkeep/command order, but did not execute this prompt.
The seven-active Planet receipt likewise supplied intro. The new evidence
pins the actual suppressed call site and its body statically rather than
claiming that old controls exercised a naming dialog.

Put FIRSTBASE before the first advance and first human upkeep. Showing it
only on reaching HumanCommands would let mineral/resource posting and
possible research interactions happen before a modal the original places
earlier. Name entry does not create a second base or rerun founding.

## Research questions are conditional

The first Base/upkeep research pipeline is after this introduction boundary.
The turn-one numeric TECH loop is also after the owned-base loop. The
[first-upkeep authority contract](seven-first-upkeep-authority.md) measures
these phases and shows that the numeric acquisition shortcut can suppress
the Blind Research priorities question. Consequently there is no justified
unconditional "FIRSTBASE, then a research question" sequence. Whether a
question appears depends on research mode/state and the caller. This does
not move initial faction-file technology grants out of their earlier setup
phase, nor claim every possible technology UI caller is audited here.

## Completion and save/load suppression

No independent "introduction played" write appears in the inspected
introduction/controller sequence. After name entry, `5c6265` ORs `8c0`
into the map-view object's `+1dd70`, restoring display flags cleared for the
landing presentation; this is not an established serialized intro-complete
bit. Its subsequent calls redraw/poll and destroy local UI objects, then
return. The immediate caller does not store a dedicated intro-complete
boolean either.

On normal fresh progress the later global advance makes turn positive, so
the turn0 entry gate suppresses the intro next time. A nonzero resume cursor
also suppresses it, independently of turn. That cursor participates in
ordinary owner/upkeep scheduling, so do not relabel it as a dedicated intro
flag or infer that zero always means upkeep is pending.

Load routine `5c05e0` calls game codec `5be250` at `5c06db`. In its later
post-load continuation, `5c0941/46` assigns focused faction `9430d4` to
resume cursor `9ba55c`. With a normal positive human focus, reentering
`53be70` consequently skips the intro even if the loaded turn is0. This is
static load-route evidence; no new native save inside an active intro or
FIRSTBASE modal was performed, and not every error/network/load route is
claimed. The separate [identity/base-name contract](setup-dialog-continuation.md)
establishes that the live Base name itself is in saved Base data.

For port save continuation, distinguish "fresh introduction pending", the
remaining first-advance/upkeep work, and ordinary command resume. An explicit
saved port phase or completed-intro marker can represent that distinction;
it is a port representation choice, not a newly discovered original field.
Do not repeat the introduction solely because a restored world still has
its generated default base name, or repeat upkeep merely because naming
finished. Do not mark all pending gameplay phases complete when committing
the first name.

## Backdrop and lower-console source notes

The intro initializes `drop_pod.flc` and `drop_smoke.flc` at
`5c5e4e/5c5e6a` into351×407 bitmaps, centers on the selected existing
base/unit and performs its landing animation before FIRSTBASE. These are
landing visuals, not a PCX frame atlas for the text-entry modal. The intro
also captures the current map-view image into a temporary bitmap through
`5c5ecb/5c5eef`; it does not load a new fullscreen FIRSTBASE background.

The bottom console's established constructor is `4660d0`. At
`466882..4668a6`, width800 chooses **console.pcx** (800×257), otherwise it
chooses **console2.pcx** (1024×257), loaded with null palette destination,
offset10/count236. At1024×768 the existing measured console origin is
`(0,511)`, documented with the console's local controls in
[console geometry](minimap-console-controls-and-colors.md). The constructor
copies/scales the loaded backing through `5f2410` at `466975` into its own
bitmap, using source and destination extents. Interface button crops come
from a separate `interface.pcx` load; they are not the console backing image.

`frame.pcx` is1024×768 and has direct constructor references in other
window paths (`4534d0`, `4a4740`). This quick audit does **not** connect
those references to the generic FIRSTBASE frame; do not select that asset
as its modal border on this evidence. Exact generic modal border/background
atlas slices and their final layout producer remain open. The source-backed
near-term backdrop is the already constructed map/console scene behind the
intro and prompt, with FIRSTBASE's installed `(x10,y40,width320)` template.

## Verification

`extract_firstbase_schedule.py` retains original indexed disassemblies,
20 checked literal call/gate/store instructions, relevant filenames, and
hashes/dimensions of the referenced local assets. Reconstructing the entire
static receipt with `--verify-only` passes, including the frozen copy.
No original emulator/UI run, production edit or rendered-frame parity is
claimed. [Machine-readable phase facts](firstbase-startup-schedule-cases.json).
