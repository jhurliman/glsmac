# High Morale in the live Scout battle route

Manufacturing, design resolution and ordinary movement already admitted High
Morale. The live Scout attack route still rejected any nonzero ability word,
preventing an otherwise supported High Morale unit from fighting.

The admission check now permits passive bit `0x4000`. It retains all other
component, ability, terrain, controller and difficulty checks. Combat still
consumes the actual stored and effective morale; High Morale introduces no
additional direct strength, round or promotion multiplier. Training and upgrade
producers remain responsible for their morale changes.

The existing live hostile-battle regression adds a supplied-state control with
High Morale on both participants and the neutral control's same stored/effective
morale and RNG. It reproduces the old rejection. After the fix it preserves the
expected casualty, surviving position, damage, promotion, movement debit,
palette initialization, presentation plan, history, wrapper retirement and
actual saved Start continuation. The existing pure original-round/promotion and
invalid-input checks and defended-Base aftermath group also pass. Targeted
correctness analysis passes for the changed combat translation unit.

The pure battle arithmetic, memory ownership, casualty transaction and codecs
are unchanged; their preceding sanitizer results remain applicable. This is a
bounded Fission Gun/Scout Infantry integration, not general weapon, armor,
reactor, ability, multiplayer or campaign combat acceptance.
