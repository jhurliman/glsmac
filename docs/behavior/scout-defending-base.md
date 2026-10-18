# RE-SMAC-SCOUT-DEFENDING-BASE-001: defeating a Base garrison

An ordinary Scout wins against a Scout defending an unfortified population-2
Base, but **does not enter or capture the Base in that command**. The original
movement wrapper returns normally with result 0 after the casualty. Its attacker
is still at the source, exhausted; the next ordinary directional key emits no
movement request. [Cases](scout-defending-base-cases.json) retain all 17 rounds,
both RNG streams and the subsequent admission result.

## Admitted inputs

This uses the existing focused local two-human Librarian attack harness and
its established display/audio/event/cleanup services. Both native-allocated
Scouts use installed Infantry/Gun/Scout/Fission components, no abilities, full
10 health, stored morale 1, effective morale 1 and movement allowance 3. The
attacker is Unit 0 at (8,8), home −1. The defender is Unit 1 at (9,9), home
Base 0: it is recreated through the actual factory after the Base is admitted,
so that home comes from the factory. Both are initially unspent.

Base 0 is a supplied coherent record and matching map presence at (9,9), owner
2, former owner 2, population 2, assimilation 0, no facilities or status flags.
This is a resolved Base fixture, **not a founding execution**. The wrapped map
is 128×64, adjacent clear flat land, without sensors or improvements. Projects
are absent; both factions own only Ecology. Retained morale and faction combat
modifiers are neutral, FANATIC is off, mutual Vendetta is set, turn is 6, both
human/active masks are 6, and focus/controller/active faction are 1.

Initial CRT state **4** reuses the earlier natural winner
`SCOUT-COMBAT-0007`; shared state is **3**. No outcome or random value is
supplied. The initial seed-3 Base assault lost and is preserved separately as
a diagnostic. It is not substituted into the victory acceptance case.

## Actual strengths and modifier order

The original attack and defense helpers query morale in **mode 1**. Both
return effective morale 1; the defender's actual home has no Crèche or riots.
The separate defense-strength morale floor remains relevant generally, but
does not change this value of 1.

Neutral conventional Gun/Scout strength is `32 * (6 + 1) = 224`. With the
installed rules, Infantry attacking a Base receives **25% attack bonus**, and
the unfortified Base receives **25% intrinsic defense bonus**. Original final
health/strength setup produces:

| Side | Initial health | Strength before partial-movement scaling | Round width |
| --- | --- | --- | --- |
| Attacker | 10 | 280 | 280 |
| Defender | 10 | 280 | 280 |

The full movement allowance is available, so no partial-attack reduction occurs.
The bonuses change modulo widths even though they are equal on both sides:
do not assume this fight must reuse the flat-ground width-224 round outcomes.

Static composition of FANATIC and Infantry-versus-Base is **sequential**, with
integer truncation after each percentage stage:

```
mode-1 morale -> conventional attack strength
             -> truncate(strength * (100 + FANATIC%) / 100), if enabled
             -> later Infantry-versus-Base percentage with another truncation
```

For this same neutral morale-1 Scout, FANATIC enabled would produce
`224 -> 280 -> 350` against the Base, not additive `224 * 150% = 336`.
The unfortified defense remains `224 -> 280`. This combined FANATIC/Base value
is **static evidence**, supported by the existing separate parsed FANATIC
flat-ground case; no new FANATIC/Base runtime matrix is claimed. Morale is
resolved before these percentage stages, and partial-movement scaling belongs
to the later battle setup. Do not merge the two percentage divisions into one
rounding step.

The reached conventional Base attack gate requires a valid defending Base,
a non-native conventional attacker on a land chassis with raw movement 1,
and the ordinary non-Psi context. The unfortified Base fallback is reached
without Perimeter/Tachyon or owner-wide defense project effects. Alternative
fortification paths are visible statically but are not validated by this case.

## Rounds, casualty and RNG

Seventeen native rounds deal ten hits to the defender and seven to the attacker.
Each round takes an attacker CRT draw followed by a defender draw, reduces each
raw output modulo 280, and hits the defender only when the attacker result is
strictly greater. A tie hits the attacker. The lethal defender hit ends drawing.

The attacker then promotes stored/effective morale **1→2**. Its promotion
weights are **84/280**; effective morale 1 admits promotion without an outer
draw, and the helper increments without a parity draw. All **34 CRT draws**
belong to rounds; state advances **4→3665287870**.

This fresh presentation context also performs one actual shared draw with bounds
`[0,65535]`, output **9605**, state **3→1018897798**, during first-use palette
initialization. The native presentation-initialized flag changes false→true.
This is recorded for exact replay of this fixture; it is **not a requirement
to add one shared gameplay draw to every battle**. No additional RNG is consumed
by the post-core cleanup or subsequent refused key request.

The defender Unit 1, owner 2, home 0 is deleted. Attacker Unit 0 survives at
**(8,8)**, home −1, damage **7**, stored/effective morale **2**, spent **3**.
It remains selected; the secondary reference to the defender clears. Its stack
links are empty. Destination Unit presence clears while the Base remains.

Base identity, owner 2, population 2, former owner 2, assimilation 0 and facilities
remain unchanged. Native aftermath adds stored status mask **8**; its wider
lifetime/meaning is not identified here. Native visibility work also adds
captor discovery and remembers owner 2. The Base record therefore is not
byte-for-byte unchanged. There is no capture entry, population decrement,
faction elimination or restart in this command.

## Why capture needs another action

The movement body snapshots two Units at entry and finds one after battle. It
takes the actual count-change cleanup and early return 0; it does not fall
through the same-count movement/capture tail. No movement result is supplied.
Returning 0 does not undo the completed battle.

After that normal return, the original allowance helper returns 3 and spent
movement remains 3: remaining movement is 0. The original selected-unit key
gate returns not actionable, shows only its already-established cannot-act
notification, clears the pending action latch and **emits no second command**.
Unit, Base and RNG state remain unchanged by that admission attempt.

Capture requires a **separate admitted action** against the now-undefended Base.
This ordinary Scout cannot perform that action again this turn; it must regain
movement, normally in a later turn. This investigation does not execute a turn
advance, spending reset, later Base capture or Base-screen construction. It also
does not infer a general no-population-loss rule for every Base battle, or an
auto-advance rule for other units, stacks, components, facilities, AI or network
contexts. Prior [undefended capture](undefended-base-entry.md) evidence remains
the separate continuation contract.
