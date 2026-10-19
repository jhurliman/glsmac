# First upkeep: faction-scoped social, TECH and command state

The original refreshes social ratings **for the faction being processed**, then processes its Bases, then handles its turn-one numeric TECH quota, then continues later upkeep planning before command dispatch. [Cases](seven-first-upkeep-authority-cases.json) include all seven command boundaries, all social vectors, research priorities, regional retained inputs, production baselines, complete decoded Unit control fields and University phase snapshots.

The initial seven-faction setup and first human boundary execute normally. Later boundaries use separately admitted next-slot continuations with the real controller frame, leaving human commands and computer turn bodies unexecuted. These are precise upkeep/dispatch controls, not an uninterrupted ordinary first turn. No new gameplay or technology choice is answered.

## First social refresh

At the completed turn-zero setup, all seven factions have their parsed intrinsic SOCIAL vectors, neutral model fields, and **zero current, retained and third produced vectors**. Founding has already used that earlier current state. At each faction's first upkeep, the native refresh produces all eleven current and retained ratings before its Base upkeep. In this neutral-model control the third produced vector also equals the intrinsic vector.

The intrinsic publication occurs inside each owner's initializer `0x5c8650`, called by setup `0x5cadb0` at `0x5cb087` for human slots and `0x5cb0be` for nonhuman slots. In the Standard Rules path, Base creation calls `0x4fa490` at `0x5c9f3e`; the selected Scout/Rover factory calls `0x5d8520` at `0x5c9f4e`, then clears home at `0x5c9f5d` and calls stack reorder at `0x5c9f65`. Only later does `0x5ca14f` test the fresh-start guard (DWORD `0x959fdc` equals zero). The intrinsic block zeroes eleven DWORDs at `0x95d478 + owner*0x20bc` via `0x5ca16d`, scans parsed faction bonuses for type3 at `0x5ca187`, and adds each signed value to its indexed intrinsic axis at `0x5ca1b4`. The block completes at `0x5ca1bc`. It does not publish current/retained ratings. Definition UNIT bonus processing follows this block; the setup-wide balancing call to `0x5c7c90` is later at `0x5cb986`, after all initializers return. Thus publish intrinsic per owner after its initial Base and Scout/Rover, before definition UNIT grants and global balancing. This address ordering is checked against the original instructions; the completed setup and subsequent refresh values have the existing native receipts.

The meaningful nonzero ratings are:

| Faction | Ratings after its refresh |
| --- | --- |
| Gaia | EFFIC+2, MORALE−1, POLICE−1, PLANET+1 |
| Hive | ECONOMY−2, GROWTH+1, INDUSTRY+1 |
| University | PROBE−2, RESEARCH+2 |
| Morgan | ECONOMY+1, SUPPORT−1 |
| Sparta | MORALE+2, POLICE+1, INDUSTRY−1 |
| Believers | SUPPORT+2, PLANET−1, PROBE+1, RESEARCH−2 |
| Peacekeepers | EFFIC−1 |

At the first Gaia command boundary, only Gaia's current/retained vectors have refreshed; future factions still have zero vectors despite nonzero intrinsic values. At Hive dispatch, only Gaia and Hive have refreshed. Preserve that faction-scoped timing. Do not retroactively adjust turn-zero founding credit when the first refresh runs.

## University numeric TECH phase

University owns named Information Networks, ID2, at turn zero. Its separate numeric quota remains1. During its first upkeep:

1. Social refresh precedes Base processing.
2. Base production posts first: in this Planet control, stored minerals5→7, target/baseline Scout2, baseline minerals7.
3. The numeric TECH stage tests global turn exactly1 and enables the acquisition shortcut.
4. With no current target, the actual selector runs. In the computer control it returns Applied Physics3, temporarily publishing target3.
5. The award wrapper clears target before acquisition. Ownership becomes2/3; shortcut suppresses ordinary score changes and follow-up target selection.
6. The loop clears the shortcut. Quota remains1. Current target/cache remain−1; score, discount, deferred count, labs and fraction remain0; pending Secrets remains false.
7. Later upkeep planning runs before computer dispatch.

The temporary target is not an acquired technology until acquisition runs. The numeric quota is configuration, not a consumed/pending count. The source can award again if the same faction upkeep is invoked twice at global turn1; the scheduler/continuation phase must prevent duplicate execution rather than clearing the configured quota.

Regional inputs also have phase authority. At University's Base/research entry and TECH award, its occupied region15 has retained Base count1, known-tile count17 and strategy0. Its research priorities are Explore/Discover/Build/Conquer=`0/1/0/0`. At upkeep return, that region's strategy is8. Do not substitute command-boundary regional strategy into the earlier selector call. These counts and codes come from native producers, not faction constants or a universal21-known-tiles assumption.

## Focused-human versus computer Blind selection

A paired direct-upkeep control starts from the same real post-Gaia/Hive, pre-University world. Common inputs include scheduled faction3 and the upkeep movement-selector flag. The human variant changes human mask2→10 and focus1→3; it is not a separate fresh human-University setup.

| University role | Awarded ID in this control | Base-processing CRT draws | Selection draws | Final target |
| --- | --- | ---: | --- | --- |
| Computer | Applied Physics3 | 1 | 7 CRT | None |
| Focused human | Planetary Networks32 | 1 | 7 shared-generator | None |

Both execute the complete native Blind chooser and acquisition without a UI answer. The numeric shortcut suppresses the early priorities question. Both preserve score/discount/deferred/labs/fraction0 and quota1. The computer finishes CRT785009333/shared1; the human finishes CRT3790446172/shared3327581586, from common CRT1459234509/shared1.

Port implementations of the selector, valuation and Blind dispatch are described in [Blind research integration](../blind-research-integration.md) and [live research](../smac-live-research.md). IDs3/32 are measured outputs, not fixed University grants. Do not generalize this no-dialog result to Blind Research disabled.

## State already changed before computer command

Upkeep includes native computer planning. Reaching the main AI turn body is not the first point at which production or Unit orders can change. These are literal boundary results:

| Faction | Production target at dispatch | Minerals / baseline minerals | Baseline item | Food | Credits |
| --- | ---: | ---: | ---: | ---: | ---: |
| Gaia | Former1 | 12 /12 | 1 | 2 | 11 |
| Hive | Scout2 | 7 /7 | 2 | 1 | 10 |
| University | Colony Pod0 | 7 /7 | 2 | 2 | 11 |
| Morgan | Colony Pod0 | 7 /7 | 2 | 2 | 111 |
| Sparta | Colony Pod0 | 7 /7 | 2 | 1 | 11 |
| Believers | Colony Pod0 | 7 /7 | 2 | 2 | 11 |
| Peacekeepers | Facility−3 | 7 /7 | 2 | 1 | 11 |

Planning changes the production target after the Base's baseline commit; the baseline item remains Scout2. Do not commit a second baseline merely because planning selected another item. These targets depend on the live planning context and are not a startup-package template.

Scouts1/2/3/4/6 receive order24, status character49 (`'1'`), first waypoint X/Y, and state bit`0x40000000`. Their waypoint destinations are respectively35,35;25,35;60,22;30,26;15,73. Remaining waypoint slots stay−1; the measured control bytes stay0. Positions, homes, morale, damage and movement spent remain unchanged. Gaia Scout0, Believers Scout5 and all Colony Pods retain order0. The native Unit writer journal reconstructs every final Unit byte. Preserve the waypoint/status/state fields, not only the order byte; this handoff does not independently model the destination-selection policy.

## Caller and resume boundary

The first human command is entry`0x527eb0`, caller`0x53cb4f`. Computer boundaries are entry`0x58f6b0`, caller`0x53cb67`. Focus remains1 while scheduled faction advances. At all seven command boundaries, acquisition shortcut, faction-processing flag, movement RNG selector, resume and controller flags are0. The main control consumes222 CRT draws and no shared draws, including seven University selector draws.

The caller runs upkeep when resume is zero, or when its separate mode flag and controller bit`0x40` admit it; after upkeep it clears that bit and the movement selector before dispatch. The probes record these fields but do not execute a new native save/load-resume path. A port continuation must preserve the completed phase and canonical research/social state. A zero resume field at this observed entry alone is not a proof that upkeep should be replayed after load.

Validation checks the prior Gaia/Hive boundaries, every current/retained social axis, University phase order and ownership store, RNG recurrences/streams, production stores and all Unit writes. Frozen evidence: `tmp/reference/seven-first-upkeep-authority-evidence-20261019/manifest.json`. Check: `tmp/reference/check_seven_first_upkeep_authority.py --verify-only`.


## Port integration checkpoint

The live default SMAC startup now publishes each owner's intrinsic vector after
initial Base/Scout creation and before compensation, without exposing refreshed
current/retained ratings to founding. The owned social producer also supports an
explicit saved-owner refresh. Tests compare all eleven intrinsic/current axes at
each of the seven native command boundaries and the retained snapshot of every
processed owner, including social-record round-trip and no replay.

This does not activate the native faction scheduler. The current explicit
all-human campaign still refreshes all owners before publishing its turn. The
world-level phase guard continues to reject a mixed first round until a saved
controller cursor can prove which owners have completed upkeep. Neutral models
remain the admitted social domain; later model changes are unfinished.


Focused validation:31 social/startup/research/restore groups pass normally and
with seven affected production/test translation units instrumented by ASan/UBSan
(other linked objects are normal; leak detection disabled). Five production
translation units pass targeted correctness analysis. The native reference
checker passes verify-only; no original assets or raw memory snapshots are
included in the public numeric cases.
