# RE-SMAC-STANDARD-CURRENT-RULES-001: Standard and Current Rules authority

2026-10-18; pinned original SMAC. [27 producer cases](standard-current-rules-cases.json).
This extends the [setup choice-transfer receipt](setup-wizard-choice-transfer.md)
without changing its inputs or results. Original setup entry, rule decisions,
option commits and dependencies execute. UI answers and checkbox state are
supplied. Twenty-five cases stop immediately after rules; two continue through
actual preference writes and new-game initialization to the first faction's
early reset. No complete Planet world or rendered menu is claimed.

**The native Standard Rules low mask is `0x1a0e` (decimal 6670). Bit `0x2000`
is clear.** Use this exact rule selection rather than a display label or neutral
defaults. However, this menu operation is not a wholesale scenario/session
reset and does not overwrite remembered custom rules.

## Standard rule values

The following masks describe the primary saved rule word's low sixteen bits.
The named checkbox and stored bit have the same polarity except world survey.

| Setting | Mask | Standard choice |
| --- | ---: | --- |
| Transcendence victory | `0x0800` | Enabled |
| Conquest victory | `0x0002` | Enabled |
| Diplomatic victory | `0x0008` | Enabled |
| Economic victory | `0x0004` | Enabled |
| Cooperative victory | `0x1000` | Enabled |
| Elimination permanently removes the faction | `0x0001` | Disabled; restarts allowed by this option |
| Flexible starting locations | `0x0010` | Disabled |
| Slower research | `0x0020` | Disabled |
| Gain technology from base capture | `0x4000` | Disabled |
| Blind Research | `0x0200` | Enabled |
| More aggressive opponents | `0x0040` | Disabled |
| Initial world survey | `0x0100` | Disabled; the **No Unity Survey** checkbox is enabled |
| No scattered Unity Pods | `0x2000` | Disabled |
| Disable random events | `0x8000` | Disabled |
| Accelerated start | `0x0080` | Disabled |
| Restricted saving / Iron Man | `0x0400` | Disabled |

The two faction-randomization options are separate secondary-word bits:
personalities `0x00800000`, social agendas `0x01000000`. On an explicitly fresh
secondary word zero they are disabled. **Selecting Standard does not clear or
replace existing secondary bits**, including these two. Do not present that
preservation as a newly produced default.

The `0x2000` rule has two relevant consumers: its menu identity is **No Unity
Scattering**, while the [resource resolver](resource-bonus-resolution.md) uses
the same bit to admit automatic land deposits. Thus Standard passes
`automatic_land_resources=false`. Forced-presence land deposits and eligible
shelf deposits still resolve normally. For the unchanged raw installed Planet
map this selects the disabled row of [the import receipt](standard-planet-map-import.md):
34 nutrient, 28 mineral and 27 energy deposits. Those are raw-load resolver
counts, not a new claim about all later preparation/startup effects.

## Active rules and remembered preferences are separate

Let `A` and `B` be the active primary and secondary words, and `M` and `N` the
remembered primary rules and secondary faction-randomization preferences.
All formulas use unsigned 32-bit words.

**Play with Standard Rules:**

```
A = (A & 0xffff0000) | 0x00001a0e
B = B
M = M
N = N
remembered rule-menu choice = 0
```

Citizen difficulty does not change this Standard mask. It already contains
Blind Research. A controlled prior active low word `0xffff` becomes `0x1a0e`;
a remembered custom mask containing `0x2000` remains remembered even though the
active Standard result clears that bit.

**Play with Current Rules:**

```
A = (A & 0xffff0000) | (M & 0x0000ffff)
if accepted difficulty is Citizen (0): A |= 0x00000200
B = (B & ~0x01800000) | N
M = M
N = N
remembered rule-menu choice = 1
```

Current reads the remembered rule selection, not the preceding active game's
low bits. It retains the active primary upper sixteen bits and secondary bits
outside the two randomization flags. The literal secondary OR is not masked;
the normal remembered-preference producer restricts `N` to `0x01800000`.
Do not admit arbitrary extra bits into that preference through an unchecked
port setter. The remembered primary's high bits are excluded by this interactive
Current branch; a canary remembered value `0xabcd2000` contributes only `0x2000`.

With remembered rules zero, Current produces zero on an ordinary higher
difficulty and `0x0200` on Citizen. It does not silently substitute Standard.
The separately remembered menu default is not the remembered rule mask.

Accepting customized checkboxes writes the resulting primary word into `M`
and saves `B & 0x01800000` into `N`. Eighteen single-checkbox native controls
bind every row above and both randomization options, including the inverse
survey checkbox. Unchecking No Unity Survey sets `0x0100`; checking it clears
that bit. This receipt does not implement the full customization UI.

## What is actually saved

The normal settings writer requests profile keys **Rules**, **Semaphore** and
**Customize** from `M`, `N` and the remembered menu choice respectively. The
first two use binary digit strings. The two continuous setup-to-new-game
controls observe those actual write requests; no host profile is changed.

For example, selecting Standard with remembered primary `0x2000` and remembered
secondary `0x01800000` leaves both preferences intact and writes them back as
Rules/Semaphore, with Customize zero. Its active primary low mask is still
`0x1a0e`, and its active secondary in that control remains zero. Reusing a
saved Current preference therefore need not reproduce the last Standard game.

Game saves carry the active words in the existing general game-state serializer
block. They are distinct from the remembered menu preferences. That serializer
location is statically checked here and its ordinary file-body machinery was
already executed in [save/load evidence](gameplay-rng-save-load.md); no new full
campaign save roundtrip is claimed for this follow-up.

## Ordinary Planet campaign versus scenario/objective flags

Map origin and scenario identity are separate. The Standard button changes
the low rule word only. These relevant settings are outside that replacement:

| Setting | Authority | Effect of Standard selection |
| --- | --- | --- |
| No Colony Pods | Primary `0x00400000` | Preserved |
| Every base is an objective | Primary `0x20000000` | Preserved |
| Secret-project bases are objectives | Primary `0x40000000` | Preserved |
| Scenario identity | Secondary `0x00000010` | Preserved at selection |
| Selected-facility objective mode | Secondary `0x04000000`, plus separate selected facility | Preserved at selection |
| Individually marked objective base | Actual Base objective state | Not set/cleared by the rules menu |
| Cooperative victory | Primary `0x00001000` | Enabled by Standard; restored from remembered low bits by Current |

The subsequent **ordinary new-game initializer** replaces `B` with
`B & 0x01800000`, clearing scenario identity, facility-objective mode and other
non-randomization secondary bits. Its broader state clear resets the selected
objective-facility scalar to zero. The two continuous controls prove this at
first-faction reset. Zero there is a cleared numeric scalar, not an invented
"no facility" sentinel; the mode bit is separately disabled.

That initializer does **not** clear primary high restrictions/objective bits
in the checked prefix. A deliberate `0x60400000` primary canary (no-colony,
all-base and project objectives) survives Standard and the actual new-game
prefix as `0x60401a0e`. This is a preservation discriminator, not a normal
ordinary-world starting condition.

For the port's supported fresh ordinary Planet campaign, explicitly establish
non-scenario authority, no no-colony restriction, no global/project/facility
objectives and no marked starting Base, then apply the selected rule producer.
The result with fresh primary upper bits zero and secondary zero is primary
`0x00001a0e`, secondary zero, cooperative victory enabled and automatic land
resources disabled. Do not derive non-scenario status from `map.type`, the
Planet label, Standard selection, or the absence of a live objective Base.
If a restored/custom/scenario state cannot meet that admission contract,
preserve/reject its actual authority explicitly instead of discarding flags.

This follow-up does not trace every earlier fresh-process/session-teardown
producer that supplies primary upper bits zero. It also does not prove loaded
scenario continuation, every objective mode, a seven-faction world, scenario
editor UI or all original save frontend behavior. The setup quick-reuse flags
entry is a different branch from interactive Current and must not be treated
as this same formula; the earlier setup receipt covers its bounded behavior.
Cancellation coverage remains the earlier receipt's stated boundaries, not a
new promise of transaction rollback. Prior evidence and the paused standing
research goal remain unchanged.
