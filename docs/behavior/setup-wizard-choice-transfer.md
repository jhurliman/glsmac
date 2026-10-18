# RE-SMAC-SETUP-TRANSFER-001: setup choices into the first human faction

Status: **13/13 original-x86 controls** against the pinned SMAC reference in
the ledger. Seven interactive choices and two remembered-choice
paths run from the actual setup controller's function entry to the first
faction initializer's early reset. Four cancellation controls stop at the
original stage boundaries. See [the cases](setup-wizard-choice-transfer-cases.json).
UI services and answers are supplied; no rendered or input-driven UI is claimed.

## Interactive choice transfer

In the checked local, single-human path:

1. Build six difficulty rows in order, with values 0 through 5. Select the
   remembered difficulty as the dialog's initial value.
2. On accepting difficulty, write that value to both the active difficulty and
   the remembered difficulty immediately. It is not deferred until final setup.
3. Enter the rules dialog. These controls accept its remembered-rules branch;
   other rule choices and option normalization are outside this receipt.
4. Build seven faction rows from the already-loaded definition slots, in order
   1 through 7. Use the remembered faction slot as the dialog's initial value.
5. On accepting a faction, remember its returned loaded slot immediately. The
   normal leader-name input follows. Keeping the supplied name then binds that
   slot as the focused faction and the sole human slot.
6. Execute the actual setup configuration writer and new-game entry. The writer
   serializes the remembered difficulty and faction as their decimal values.
   The first faction initializer receives that same loaded slot; its early
   reset stores the selected difficulty for the human faction.

The row identity is a loaded definition slot, not a newly assigned sequential
human-player number. The tests give each loaded slot a distinct synthetic
definition label and observe the selected definition at the actual initializer
entry. They do not establish how real profiles were initially loaded or shuffled.
The existing faction-input contract
separately checks profile-specific grants and social inputs.

The seven accepted controls cover every loaded slot and every difficulty at
least once; they are not a full 42-combination setup/world matrix. Every such
control observes the original six/seven row IDs, remembered defaults, active
difficulty, remembered faction, focused slot, sole-human membership, actual
first initializer argument and produced human stored difficulty.

## Cancellation retains earlier accepted state

Initial state for these four controls is active difficulty 2, remembered
difficulty 5, remembered faction 7, and human/focus slot 1. Those are supplied
discriminators, not original installation defaults.

| Supplied cancellation | Original destination checked | State at that boundary |
| --- | --- | --- |
| Difficulty | Exit-cleanup path, before cleanup | Active 2 and remembered 5 remain; no faction binding or initializer |
| Rules, after accepting difficulty 4 | Difficulty stage reentry | Active and remembered difficulty remain 4; remembered faction remains 7 |
| Faction, after accepting difficulty 4 and rules | Rules stage reentry | Difficulty remains 4; remembered faction remains 7; focus/human remain slot 1 |
| Leader entry, after accepting faction slot 3 | Faction-stage gate reentry | Difficulty remains 4 and remembered faction is already 3; focus/human remain slot 1 |

No configuration serialization or faction initialization occurs before these
checked cancellation boundaries. Retained remembered values here are **in-memory
state**, not evidence of a disk write on cancellation. The controls do not
execute final exit cleanup, a complete back-and-retry cycle, or later rollback.
Do not describe the entire setup as one atomic transaction on this evidence.

## Reuse remembered choices

A separate entry mode reads remembered difficulty, rules and faction instead
of presenting the preceding choice dialogs. In the checked local single-human
path, it binds the remembered faction as focused/sole-human, serializes the
remembered choices and reaches that faction's actual early initializer.
Two controls use Citizen/loaded slot 2 and Transcend/loaded slot 7; both emit
no difficulty/faction rows and enter no choice dialogs. Invalid remembered
values, other entry modes and multiple-human/network paths remain open.

## Evidence boundary

Original controller branches, row-building loops, choice stores, human/focus
binding, configuration formatting/writes, new-game caller and initializer
execute continuously. Supplied services are UI constructors/templates/display,
faction-preview display, modal answers, leader text input, localized labels,
host profile writes/reads and file removal. Profile writes are observed requests
with successful supplied returns; no host preferences are modified or reloaded.
Citizen's existing beginner-settings preservation preference is supplied.

Definition strings and remembered rule bits are synthetic supplied inputs;
map regeneration is disabled. Execution stops after the first early faction
reset, before remaining faction setup, site selection, map generation, founding
or the first command. This receipt closes the choice-to-initializer transfer
gap in the setup evidence index, while the
governor/founding and
first-command receipts retain their own
separate coverage boundaries.
