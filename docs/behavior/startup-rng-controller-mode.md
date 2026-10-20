# RE-SMAC-START-RNG-MODE-001: produced local command RNG mode

Status: **63 original-x86 startup/controller compositions pass**, recording
literal paired RNG words, draw calls and selector writes. Seven installed
factions cross three supplied shared/CRT state pairs and prior selector0/1/99.
[Fixtures](startup-rng-controller-mode-cases.json); fingerprint in [the ledger](README.md).

This extends the [explicit RNG-state seams](gameplay-rng-state-seams.md) and
[first-command controller boundary](startup-first-command-boundary.md). It
establishes the controller-produced selector; the supplied initial RNG words
remain sensitivity inputs, not a normal new-game seed recommendation.

## Original controller selects the mode

The checked local controller explicitly writes the movement RNG selector:

1. Set it to1 before global advancement.
2. Set it to1 again before faction upkeep.
3. Set it to0 after upkeep and before entering the human map-command function.

All63 cases follow this sequence, including a non-Boolean prior value99.
The first command therefore receives selector0 from actual original controller
instructions. It is not inherited from the pristine executable image or
supplied as an assumed command-mode flag in these checks.

The movement lottery's traced consumer chooses the shared range stream for
selector0 and CRT raw-output modulo cost for a nonzero selector. This is a
movement-caller decision; it does not reroute all random calls. In particular,
[Scout combat](scout-conventional-combat.md) uses CRT under both checked selectors.

## Paired words through the checked startup composition

The supplied state pair is installed before the admitted startup stages. The
actual unit initialization, late social construction, normal balancing, local
controller, global advancement and first upkeep then execute with the previously
declared controlled-world and supplied presentation services.

In these fixtures, normal balancing makes one CRT draw, global advancement
makes four, and faction/base upkeep makes one. No shared draw or seed/exchange
call occurs in this observed interval. The literal boundary words independently
match six CRT recurrence steps; shared retains its supplied word.

For supplied shared1/CRT3, every checked faction produces:

| Boundary | Shared word | CRT word | Selector |
| --- | --- | --- | --- |
| Unit initialization and entry to balancing | 1 | 3 | Supplied0,1 or99 |
| Local controller entry | 1 | 3173050 | Prior value |
| Global advancement entry | 1 | 3173050 | 1 |
| Faction upkeep entry | 1 | 460785054 | 1 |
| First map-command entry | 1 | 1545176553 | 0 |

The fixtures also include shared0/CRT0 and shared4294967295/CRT2147483648.
They retain literal observed words at all six boundaries. The private receipt
additionally retains individual draw calls and every word/selector write.

## What this does not establish

The [same-guest movement extension](startup-first-movement-rng.md) now invokes
the real first movement wrapper on the startup-produced Peacekeeper Scout after
this boundary, retaining actual mode0 and paired words. Five cases verify one
shared draw, CRT preservation and actual spending/coordinates. Input-loop,
normal local presentation and full seed origin remain separate.

The six-draw count belongs to this controlled single-active-human world and
the stated service boundaries. It is not a universal normal-game startup count.
Intro/UI, token/model/site inputs, region aggregation and visibility services
retain their prior supplied scope. Other factions, full generated terrain,
intro decisions and later actions can add work and draws.

The full frontend/new-game setup, clock-to-world seed chain, two successive
new games in one persistent application, input-dispatched movement/combat,
map-command input body, network controller and save/load frontend remain open.
The separate controller entry that seeds CRT from the map offset is not the
local controller used here; do not turn that different path into a universal
local new-game seed rule.
