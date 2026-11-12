# RE-SMAC-NATIVE-OFFENSE-RETREAT-001: bounded native offense and land retreat

Status: **22 continuous original birth→native-attack sequences pass**, with252
independently checked combat rounds and539 continuous command CRT draws. Thirteen
end with both units alive after retreat, three with a native lethal victory and
six with a human defensive victory. All44 creation/attack wrapper calls return
normally. This is a stable bounded checkpoint; ordinary visibility producers are
the next research priority.

The [22 numerical fixtures](native-offense-and-retreat-cases.json) extend
[initialized native birth](native-birth-producer.md). A component-complete,
fully initialized Mind Worm at `(17,17)` attacks the actual Spartan starting
Scout Rover at `(16,16)` on a supplied128-by-64 interior land world. The remote
human HQ is at `(40,16)`. Native references are inactive after their actual reset;
the checked encounter consumes one CRT draw before the later attack.

The complete original region builder runs after terrain variants and preserves
both RNG states. Component numeric reads are checked against the accepted
original-loader packet, including armor. No battle-strength, retreat, damage,
movement-spending, promotion or removal helper returns are supplied.

## Strengths and rounds

In this no-home, no-project, neutral-opponent profile, the human effective morale
is2. The native's effective level follows the existing current-turn schedule,
independently of its stored morale byte. For the checked reactor1 Psi fight:

```
native initial attack strength = 3 * (6 + native_effective_level) * 32
human defense strength        = 2 * (6 + 2) * 32 = 512
```

The native attacker's round width is divided by3 before turn15. It is unchanged
at15 and later. Thus turn14 uses192 against512, while turn15 uses576 against512
at native effective level0. This differs from the previously checked early-turn
reduction when the native is the defender.

Each round uses two ordered CRT draws, each reduced modulo its own side's width.
The attacker hits only when its result is **strictly greater**; ties hit the
attacker. Each hit adds one damage in this reactor1 profile. Do not substitute
a single combined-probability roll for these two draws.

Battle-turn inputs14,15,16,45,90,170,250 and330 are explicit diagnostic counter
changes after birth at turn1. These are not simulations of elapsed native AI
turns or proof that its broader runtime stays unchanged during ordinary play.

## Retreat timing and movement allowance

In the checked unstacked, non-base land profile, the faster defender can arm a
retreat countdown. The comparison uses damage-adjusted full movement allowances
with the elite addition suppressed, **not remaining movement after spending**.

- Healthy Rover allowance6 exceeds the Mind Worm's3 and arms retreat.
- Infantry Scout allowance3 ties the attacker and does not arm it.
- A Rover with two initial damage still has allowance6 and arms retreat.
- A Rover with eight initial damage has allowance3 and loses the advantage.
- A healthy Rover whose spent counter is already6 still retreats; its spent
  counter remains6 afterward. Zero-spent controls likewise remain at zero.

The armed countdown is the ceiling of the defender's initial remaining health
divided by2. It decreases with damage from successful attacker hits while the
defender remains alive. Healthy10-health defense attempts retreat after five
additional hits; the initially damaged8-health defense attempts after four.
The check is not triggered merely by round count or damage suffered by the
attacker. Additional eligibility gates for other designs, abilities, stacks,
bases and modes are outside this checkpoint.

## Direction selection and blocked routes

For the attack from southeast toward `(16,16)`, the preferred escape is straight
away to `(15,15)`. Candidate directions receive a circular directional separation
score from the direction back toward the attacker. Only scores above2 qualify;
the best strict improvement wins in the original clockwise neighbor scan.

The controlled routes demonstrate the resulting order:

| Changed escape terrain | Original retreat destination |
| --- | --- |
| Ordinary land | `(15,15)` |
| `(15,15)` is sea | `(14,16)` |
| `(15,15)` and `(14,16)` are sea | `(16,14)` |
| All three are sea | No retreat destination; combat continues |
| `(15,15)` is land fungus, with no protective project | `(14,16)` for this Rover |

The two side routes have equal directional score; the earlier scanned west
route wins that tie. Original map/domain/other candidate helpers execute for
every route. This does not specify all their occupancy, diplomacy, pod, project
or special-design rules. When the armed attempt finds no valid destination,
the checked combat does not repeatedly attempt retreat on later hits.

## Aftermath is separate from retreat and promotion

The baseline seed19/turn15 fight has13 rounds. The human retreats to `(15,15)`
with damage5. At combat exit the native has damage8; its stored morale changes
from1 to2. Later, the native-owner movement aftermath repairs **one** damage
point because its origin `(17,17)` contains land fungus, leaving damage7.

A paired control clears only that origin fungus after birth. It has identical
rounds, RNG, retreat and stored-morale change, but native final damage stays8.
The repair therefore is not a promotion heal. Positive-damage native lethal
victories also receive this repair in the checked profile; zero damage stays
zero rather than underflowing.

The native remains at its origin after either retreat victory or a lethal
victory. It spends3 movement points. Retreat moves the human through the actual
stack/move helper and preserves its existing spent value. The legacy attack
request returns0 in all22 cases, including successful native attacks; it is
not a victory/defeat Boolean. Real casualty removal/compaction executes when
one side dies.

All six checked human defensive victories produce no treasury reward, unlike
the earlier offensive Rover-versus-native bounty cases. This observation does
not replace the broader reward policy for other contexts.

## Promotion draw order

Promotion uses battle health **before** the later fungus repair. The fixture
records each side's computed admission weight and every consumed CRT draw.

For these reactor1 fights, attacker-victory weights are the attacker's remaining
health-scaled strength and the defender's initial-health/max-health-scaled
strength. Defender-victory weights instead retain the attacker's initial-health
weight and scale the winner by its remaining/initial health. Thus an initially
damaged defender must not be normalized with the healthy denominator by default.

At wild effective levels0/1, native promotion admission needs no extra draw.
Later controls execute the weighted admission draw; the reached level3 helper
also takes its separate half-chance draw. The effective-level6 control can pass
admission while its stored morale remains1 because the promotion helper's level
cap prevents an increment. Stored native morale and current-turn effective level
remain separate quantities.

## Explicit scenario inputs and limits

Variants are baseline; clear native-origin fungus; the three sea-route patterns
above; fungus on the straight escape; an explicitly supplied Infantry defender
design; initial defender damage2 or8; damage2 combined with all three sea routes;
and the fully spent defender. The Infantry variant supplies a canonical installed
design to the same human unit after birth; it is not a different faction-startup
producer. Terrain, damage, spending and turn variants are declared inputs.

This checkpoint covers Mind Worms against this Spartan ground-unit profile,
reactor1, no home modifiers, projects or special abilities and native-life1.
General retreat eligibility, other weapons/reactors/domains, tactical AI orders,
capture/ownership changes, full later native-reference policy and live rendered
command admission remain open. Source services and address-level evidence stay
in the separate private audit.
