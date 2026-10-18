# Base Governor accepted command contract

2026-10-18, stock SMAC. This combines the existing830 settings-conversion
checks,22 reset/intake checks and six startup/founding cases with new remembered
configuration storage controls and **four accepted Apply compositions executing
the actual production, recalculation and citizen follow-up bodies**.
[New literal cases and mapping](base-governor-command-handoff-cases.json).
Prior [settings/reset evidence](governor-settings-and-worker-reset.md) is reused,
not rerun. SMAX command parity is unestablished.

## Advanced control order and saved masks

The dialog checkbox word uses consecutive bits0..18 in this exact order.
The saved Base configuration uses the independent masks below. Decode and
encode explicitly; the saved word is not the checkbox word shifted as a group.
Treat full configuration words as unsigned32 bitmasks, including activation.

| Control index, zero-based | Meaning | Saved configuration mask |
| ---: | --- | ---: |
| 0 | Governor active | `0x80000000` |
| 1 | Allow multiple or no priorities | `0x00200000` |
| 2 | Automate newly built units | `0x00000080` |
| 3 | Manage citizens and specialists | `0x00000040` |
| 4 | Manage production | `0x00000001` |
| 5 | Produce scouts | `0x00400000` |
| 6 | Produce land combat units | `0x00000200` |
| 7 | Produce naval combat units | `0x00000400` |
| 8 | Produce air combat units | `0x00000800` |
| 9 | Produce land defense units | `0x00001000` |
| 10 | Produce air defense units | `0x00002000` |
| 11 | Produce prototypes | `0x00080000` |
| 12 | Produce transports | `0x00800000` |
| 13 | Produce probe teams | `0x00100000` |
| 14 | Produce terraformers | `0x00008000` |
| 15 | Produce colony pods | `0x00020000` |
| 16 | Produce facilities | `0x00010000` |
| 17 | Produce Secret Projects | `0x00040000` |
| 18 | Hurry production | `0x00000020` |

Explore/Discover/Build/Conquer are separate saved bits24/25/26/27, respectively.
Accepted advanced Save preserves those four bits (`0x0F000000`), replaces all
19 options from the submitted checkbox selections, and clears unrepresented
configuration bits. It copies the resulting full word into the **owner faction's
remembered Governor configuration**, even when inactive or configuration-only.
No normalization forces activation/citizen management or removes independent
permissions in this accepted conversion. Earlier UI constraints are separate.

## Local accepted Apply

Use the newly committed options and preserve this order:

1. With activation and production management enabled, clear the separate Base
   status high bit and clear the production queue count. Request actual
   production selection for this Base with the immediate-management argument1.
2. With activation and citizen management enabled, clear worked-square mask,
   retained specialist count and provisional specialist count. Force the full
   Base recalculation, then run the citizen-management follow-up.
3. The outer command requests map refresh unless activation and production
   management are both enabled. The production selector can request its own
   refresh; skipping the outer request does not mean no refresh occurs.

Unchanged active citizen settings still clear allocation and run the follow-ups.
Inactive Governor options do not enter these active-management branches. The
configuration-only method form stores Base options and remembered template but
skips all these immediate actions. It is not the complete all-Base bulk command.

Four new native accepted-setter compositions use a small unmanaged human Base,
Discover retained, a complete initial worker allocation, no riot, and all
production permissions disabled:

| Apply form | Observed actual result |
| --- | --- |
| Active, citizens only | Reset fields are0 before forced recalculation; native allocation rebuilds; citizen follow-up runs; target and queue remain. |
| Active, production only | Queue count1→0 and status high bit clears; native selector chooses Stockpile. It performs internal resource recalculations and map refresh. |
| Active, production and citizens | Same production result, then the explicit allocation reset, forced recalculation and citizen follow-up. |
| Configuration-only, both management options | Both Base and remembered settings commit; target, queue, status and allocation preserve; no immediate callbacks. |

The fixture's resulting intake is3/5/2 and worked mask3 in the three local
forms; the configuration-only form leaves its initially uncalculated intake
unchanged. This is a literal small-profile outcome, not a general tile-choice
or Governor production policy. Existing reset/intake evidence separately covers
an allocation switching to an energy tile. Production selection itself can call
recalculation repeatedly before the later explicit citizen reset: do not assume
the first nested recalculation is that reset's callback.

## Remembered faction configuration lifetime

The remembered template is saved faction state, distinct from each Base's
current settings and from dialog scratch. Fresh initialization produces raw
`0x00DFBE41`, or `0xC0DFBE41` at global difficulty0; the existing six startup
cases establish the difficulty producer and first Base copying its template.
Keep the full raw startup word, including bits not represented by the advanced
checkboxes. A later accepted advanced Save deliberately reconstructs the word
under its different mask rule above.

New distinct all-eight-slot controls show the whole-new-world parent preserves
templates until first-child entry. The tested fresh owner1 initializer replaces
its template with the produced default; peer/slot0 canaries remain. A nonzero-
turn owner1 initializer preserves its template. Do not bulk reset all templates
at parent entry or treat first-child observations as eight completed resets.
Two complete native ordinary save/load roundtrips restore all eight poisoned
templates exactly. Each Base retains its separate configuration authority;
changing the remembered template is not an implicit all-Base Apply.

## Command boundary and remaining gaps

An implementation can now serialize the exact19 selections, commit the remembered
template and dispatch the measured immediate callbacks through existing engine
authorities. Save must publish resulting target/queue/allocation/resources,
rather than only changing checkboxes. The full wrapper performs a cached Base
recalculation before permission/dialog handling; complete opening/Cancel cannot
be promised to be a no-op from this accepted-setter receipt.

These new controls enter **after acceptance** and stop after the immediate
callback branch. Ownership/stale-state admission, complete native dialog
lifecycle, network synchronization, all-Base iteration, every production
permission's policy, riot/specialist optimization and complete specialist/Psych
consequences remain distinct. The four new compositions exercise native
citizen follow-up only in the declared no-riot profile and production policy
only with permissions disabled. No new broad Governor AI algorithm, artwork
or live Inspector parity is claimed. Standing research remains paused.
