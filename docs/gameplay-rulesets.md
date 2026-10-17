# Gameplay ruleset identity

New games explicitly choose **Alpha Centauri** (`smac`) or **Alien Crossfire**
(`smax`) before initialization. Both are still in development. This selection
identifies the intended gameplay rules and selects the available faction roster;
it does not establish that either game's mechanics are faithfully implemented.
SMAC offers the original seven factions; SMAX also offers the seven expansion
factions. Future verified gameplay rules can branch on this identity.

The Start Game, Quick Start, and multiplayer host menus show the chooser.
The lobby displays the chosen identity read-only. Return to the main menu to
select another identity for a new session.

Command-line quickstart and hosting require an explicit choice:

```
GLSMAC --quickstart --ruleset smac
GLSMAC --host --ruleset smax --gamename "Planet" --playername "Host"
GLSMAC --join localhost --playername "Guest"
```

`--smactype gog|loki|steam|legacy` describes the installed asset distribution.
It does not choose gameplay rules. Joining clients use the host's identity;
combining `--join` with `--ruleset` is rejected. Existing asset installation
requirements are unchanged.

## Synchronization and scripts

`GlobalSettings` starts with no selected identity. Initialization and serialization
reject that state. The controlled `glsmac.select_ruleset(id)` binding accepts only
`smac` and `smax` before initialization; generic settings writes cannot change it.
`game.get_ruleset()` reads the current identity. The `configure_state` event also
provides `ruleset` when configuring a new host or single-player faction roster.
Joining clients receive the host's faction definitions instead of configuring a
local roster.

The `State` settings payload sent in `PT_GLOBAL_SETTINGS` carries the identity.
Clients defer the `game_state` notification until they accept these settings,
including when joining an already-running game. This matters because the server
sends the state notification before its settings. Later settings updates cannot
change the identity of an initialized session.

Global settings format version 1 starts with a typed integer version and a
validated ruleset string, followed by the previous settings fields. Legacy
unversioned settings and unknown versions/identities are rejected before applying
settings. Old string-first readers likewise reject the new integer-first format.
Mixed-version peers cannot join. This is a development protocol change, not a
general saved-game migration or top-level network version negotiation. Terrain
`.gsm` files do not carry gameplay-session settings.

The early rejection guarantee covers the version and ruleset identity. Decoding
the remaining map/rules/settings and faction bodies retains the existing
nontransactional behavior; malformed bodies may partially apply before failure.

## Regression coverage

Native tests exercise real `State`, `Packet`, `Client::ProcessEvent`, and GSE
wrappers: both identities roundtrip, legacy/unknown headers reject without
changing settings, frozen identities reject changes, and successive lobby/running
joins wait for the new host identity. Script tests execute the production faction
roster and chooser; CLI tests execute the real argument parser without game assets.
These checks do not claim a graphical multiplayer playthrough or gameplay fidelity.
