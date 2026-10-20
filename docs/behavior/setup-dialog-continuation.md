# RE-SMAC-SETUP-CONTINUATION-001: faction display, identity edits and first-base naming

Status: **static pinned-executable and installed-asset contract**. No new
original-x86 execution, rendered dialog or custom-name save/load round trip is
claimed. This extends the 13 original controls in
[setup choice transfer](setup-wizard-choice-transfer.md), which supplied modal
answers and deliberately skipped customization. The source receipt contains
full indexed-function disassemblies, seven faction asset identities/hashes,
the three display text blocks and the five installed Script templates.

## Faction selection and portrait

`5a0b80` builds loaded slots 1 through 7. Each row is constructed as
`formalName + "|(" + descriptor + ")"`; `|` is an actual UI text separator,
not an ordinary space. The formal name is definition offset `+384`, the
descriptor is `+3c4`, and the definition stride is `59c` hex. The selected row
is the loaded slot identity. `5a168d` installs `5a06d0` as the selection
callback and calls it once for the initial remembered/focused selection.
Changing the highlighted faction therefore has an existing preview producer.

The installed `#FACTION` template specifies width 350, x/y -5, caption
`CHOOSE A FACTION`, a listbox and `OTHER`. Preview `5a06d0(slot)` uses that
slot's definition filename at `95a4cc + slot*59c`:

| Preview mode `94f4a4` | Text loaded from the faction TXT |
| --- | --- |
| zero | `#BLURB`, the quotation and attribution |
| nonzero | `#DATALINKS1`, followed by `#DATALINKS2`, the leader/background/agenda and faction rules |

The preview caption comes from the selected formal name. Faction-dialog
secondary action 2 toggles this mode and returns to the faction stage. Action
3 is `OTHER`: accept a definition filename, load its definition (`59d1d0`) and
graphics (`45eb70`), and reenter selection. This is a data-driven file choice;
customizing a name does not select a different rules profile.

For all seven installed faction PCXs (`GAIANS`, `HIVE`, `UNIV`, `MORGAN`,
`SPARTANS`, `BELIEVE`, `PEACE`), the **setup portrait source rectangle is
x=304, y=457, width=131, height=157**, or half-open bounds
`[304,435) × [457,614)`. Each sheet is 1024 × 768, 8-bit, one plane.
`45ed35..45ed58` passes source image `7c9c48`, transparent palette index 9,
these coordinates/dimensions, and one zero argument to sprite extractor
`5fc7d0`. The extractor checks x+width/y+height against image dimensions,
stores width/height, and compares source pixels with the supplied key.
Keep palette index 9 transparent during import; do not derive transparency
from an assumed RGB color. Original sprite extraction can trim transparent
rows internally; the rectangle above is the atlas input extent.

The resulting sprite is `7c6e00 + slot*65c` hex. `5a07de` attaches it to
picture control `94bb28` with two zero arguments. `94ba2c` receives the same
sprite pointer only when `94bb5c == 0`. The preview calls `61af30(50,50)`
and `61aed0(500)`. The latter stores 500, or 750 when the dialog's `a14`
field is zero, flag `400` is clear and global display width `9c0004 >=1024`.
These are **source layout parameters**, not a verified final screenshot or
CSS coordinate prescription. `61dbd0` receives objects/values from
`94f404,94f3fc,94f400,0`; their final rendering semantics remain open.

## Leader name and customization are live identity data

The normal accepted selection opens `#NAME` when definition DWORD `+0`
is zero, otherwise `#NAME2`. Both templates have width 440 and x/y -5;
their row labels are `Name (Male):` and `Name (Female):`. Both include
`Change Gender`. The default name comes from `+34`, and the passed maximum
is 23 bytes. A nonzero modal return goes back to faction selection. On
acceptance, the NUL-terminated result at `9c39f4` is copied into `+34`.

The modal wrapper `641090` transfers the dialog's `+3100` secondary-action
field to global `9c4550`. After the accepted name copy, zero skips extended
customization; nonzero enters `#CUSTOMNAMES`. Other nonzero selection actions
can also enter customization. Thus the source does **not** establish an
unconditional three-dialog sequence for every normal acceptance.

`#CUSTOMNAMES` specifies width 440, x/y -5, caption `Customize Your Faction`
and `Restore Defaults`. The seven rows are registered in this exact order:

| Row | Definition offset | Passed maximum | Accepted result |
| --- | --- | --- | --- |
| Name | `+34` | 23 | `9c39f4` copied to definition |
| Formal | `+384` | 39 | `9c3af4` copied to definition |
| Noun | `+2e4` | 23 | `9c3bf4` copied to definition |
| Adjective | `+304` | 23 | `9c3cf4` copied to definition |
| Title | `+4c` | 23 | `9c3df4` copied to definition |
| Description | `+64` | 23 | `9c3ef4` copied to definition |
| Gender | stack scratch initialized from localized Male/Female | 23 | See edge case below |

Those offsets are relative to `95a4c8 + slot*59c` and include neither a new
faction ID nor changes to the parsed faction bonuses. Negative modal return
goes back to faction selection before the six copy operations. A nonzero
secondary-action field reloads that definition from its file, reloads its
graphics and rebuilds customization; this is the Restore Defaults path.
For slot zero that reload is skipped. The earlier accepted leader-name copy
has already happened before customization; cancellation is not a proven
rollback of the whole setup transaction.

**Gender edge case:** `5a1d28..5a1d58` compares the stack scratch's first
character, case-insensitively, with the localized Female string and stores
1 on equality, 0 otherwise. `66fc30` is the bounded case-insensitive compare,
with length 1 here. The static body does not copy the seventh global result
`9c3ff4` back to this scratch. `62c280` clones each default into allocated
dialog storage rather than retaining the original default pointer. Therefore
this trace cannot certify that editing the Gender row changes the final
gender. Do not report a successful gender toggle from these receipts. A
native UI/input control is still needed to resolve the observed behavior.

After acceptance the existing controller binds the selected loaded slot as
focused/sole human, writes remembered setup settings and enters `5cadb0`.
The live initializer `5c8650` consumes the same definition strings, including
leader/noun duplicate-name disambiguation; accepted edits are not discarded
into a presentation-only copy. This is static continuity, extending the
previous measured slot-transfer controls, not a new customized full startup.

The ordinary game codec `5be250` transfers `0x2ce0 = 8 * 0x59c` bytes
starting at `95a4c8` in its read/write branches, including all these identity
fields. Persist the edited identities in port game state/save data. The
configuration writer's remembered slot/difficulty does not replace saving
the edited record. A customized identity round trip remains unmeasured.

## First-base naming is a later command on an existing base

`#FIRSTBASE` is referenced by introduction routine `5c5b30`, not the base
name generator `4f9950`. The routine scans live bases for the supplied faction
and selects its first matching base. When found, local flag `[esp+38]`
remains zero. The no-base fallback sets that flag to one and looks for an
owned unit; that path skips the first-base name prompt.

At `5c61c1`, the existing-base path takes its current name from
`96f1c3 + baseIndex*130` hex, i.e. live Base `+13`. It copies that name as
the default, initializes `SCRIPT/FIRSTBASE` with maximum 23 and flags 4,
and runs the modal at `5c622c`. The next instructions copy `9c39f4` back
to that **same live Base name** and call `50cc80(baseIndex)`. There is no
controller branch on the modal's returned EAX at this point; any cancellation
or empty-name policy inside the generic modal remains untraced here.

`50cc80` counts other bases with the exact same case-sensitive name. If at
least one exists, it truncates the accepted name to at most 21 bytes, appends
`-`, then appends decimal `(matchingOtherBases + 1)`. This is a single pass,
not a search for the next unused suffixed name. No duplicate leaves the name
unchanged. This normalization happens after the dialog copy.

The installed template specifies x=10, y=40, width=320, caption
`Enter a name for your first base.` and label `Name:`. The ordinary codec's
Base-array transfer includes Base `+13`; the name is saved with the base.
The first-command receipt previously supplied this whole introduction, so
it did not validate this modal or the renamed result.

For the port, create the starting world/bases with their generated names,
then apply accepted first-base naming to the selected human's existing base
through the game engine. The prompt must not create a second base or change
the starting production/resource ordering. Save and inspector/map consumers
should read the same live name and edited faction identity.

## Evidence and remaining limits

The expanded static extractor checks 33 literal instructions against the pinned PE,
validates all seven PCX headers, and records exact installed text blocks and
asset SHA-256s. The [TXT binding addendum](setup-identity-txt-bindings.md)
pins each customization default and the Restore Defaults name replacement.
[Machine-readable facts](setup-dialog-continuation-cases.json)
distinguish static facts from the earlier supplied-UI execution coverage.
No foreground app, emulator session, installation or production code changed.
Final screen placement, actual input validation/cancellation, edited gender,
customized identity save/load, and complete introduction rendering remain
open; none prevents wiring the source-backed identity/base-name commands.
