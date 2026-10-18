# Live ordinary Scout battles

The local native `move_unit` route now resolves a foreign singleton Scout on an
adjacent clear flat land square. It uses actual owned Infantry/Gun/Scout armor/
Fission components, mode1 effective morale, damage, home state, remaining
movement, and the uncapped FANATIC profile flag. The admitted campaign is local
single-player with two human participants at Librarian, ordinary no-project
movement, and no existing protected diplomacy. This is a controlled combat
slice; other designs, terrain, partial-movement confirmation, Base capture,
strategic AI, network play and victory remain separate work.

Directional keyboard input sends conventional Units through this native move
route even when the frontend sees an enemy. Legacy native-life combat retains
its separate script event. The queued callback carries scalar Unit IDs and
destination coordinates and resolves current entities before dispatch.

The existing copied-RNG combat kernel feeds the live survivor, casualty,
home-Base refreshes, remembered occupancy and ordered Unit roster. These publish
together before despawn hooks. A second queued direction for a killed Unit is
ignored through its retired native wrapper; it cannot bind to a compacted
survivor. An attacking winner stays at source. An Elite winner can issue the
next normal move into the emptied square without replaying combat.

Battle history is independent of the surviving roster. The admitted hostile
caller sets mutual primary words, retains transition dates and integrity, and
increments winner-to-loser paired counters and the victim's design loss count.
Nonzero faction initialization clears the measured mutual primary mask while
retaining the remaining history. Both current live restart consumers apply it.
Simulation V12 saves this state with bounded sparse loss records; V11 campaigns
migrate to pristine history because that version had no native battle writer.

The original lazy hit-palette identifier consumes the shared stream only on
first initialization. Its lifetime belongs to the engine rather than the
campaign: loading a save preserves the current process identifier while
restoring campaign RNG and histories. New-world secondary relation rows retain
the engine's last values under an explicit port policy. Original evidence covers
the checked initialization prefix, not every reused-process frontend path.

Evidence:

- [Actual hostile movement](behavior/human-scout-hostile-movement.md)
- [Mutual hostile setter and repeated attacks](behavior/hostile-movement-war-transition.md)
- [Mode1 morale, home effects and FANATIC](behavior/focused-scout-battles.md)
- [Initialization, save and palette lifetimes](behavior/hostile-state-initialization-lifetime.md)
- [Nonzero restart history](behavior/restart-hostile-history.md)

Native integration checks exercise two consecutive ordinary battles, existing
Vendetta, an Elite follow-up move, defender victory with duplicate queued input,
homed negative morale, parsed FANATIC, casualty hook coherence and actual Start
restore with a different process palette. Codec checks include all 512 loss
counters, old-version migration and malformed history rejection. These checks
do not establish rendered combat presentation or SMAX-specific rules parity.
