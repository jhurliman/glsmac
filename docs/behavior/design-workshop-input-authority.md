# Workshop producer input authorities

2026-10-20. Same pinned original as the
[human producer](design-workshop-human-producer.md). This is a semantic mapping
addition; the already frozen arithmetic and native expectations are unchanged.

**Corrections:** `95d1ac` is rank position; `95d584..594` are current AI
priorities; definition field `95a9a0` is MORALE bonus. They are not Base count,
a current social vector, or a personality header field respectively. The
reference argument named `personality` is a naming error: pass the installed
**MORALE bonus** to it. The negative-`personality` control actually varies that
MORALE bonus to -1. Its numeric check remains valid.

All Faction addresses below use `address + owner*20bc`, relative to raw Faction
base95d1a8. Definition addresses use `address + owner*59c`, relative to installed
definition base95a4c8. They are separate record families.

| Address | Offset | Actual authority | Port mapping |
| --- | --- | --- | --- |
| 95d1ac | +004 | Retained rank position, 1..7 after ranking update;7 strongest | Ranking result retained by the original update lifecycle |
| 95e200 | +1058 | Retained owned Base count | Existing retained `base_totals` count, matching its production timing |
| 95d584 | +3dc | Current AI fight/aggression priority | Retained/current faction AI policy; **not POLICE** |
| 95d588 | +3e0 | Current AI wealth/Build priority | Faction AI policy; **not GROWTH** |
| 95d58c | +3e4 | Current AI power/Conquer priority | Faction AI policy; **not PLANET** |
| 95d590 | +3e8 | Current AI growth/Explore priority | Faction AI policy; **not PROBE** |
| 95d594 | +3ec | Current AI tech/Discover priority | Faction AI policy; **not INDUSTRY** |
| 95a9a0 | definition+4d8 | Parsed faction MORALE bonus | Faction file `MORALE, signed_integer`; default0 only when the original loader default applies |
| 95d404 | +25c | Current social MORALE rating | Current social frame, axis4; its additional gate is bypassed for the human owner |
| 95d418 | +270 | Current social INDUSTRY rating | Current social frame, axis9; read by fast-land High Morale gate |

The current social vector starts95d3f4, with the installed order ECONOMY,
EFFIC,SUPPORT,TALENT,MORALE,POLICE,GROWTH,PLANET,PROBE,INDUSTRY,RESEARCH.
Retained95d420 and third95d44c vectors are different authorities. The human
Workshop does not replace its stored AI priorities with human governor's zero
or directive-derived effective worker-scoring weights.

## Producer evidence

`5c4050@5c4290..5c42dc` assigns rank positions7 down to1 and writes
`95d1ac+owner*20bc` at5c42d2. This is the rank lifecycle already documented in
[ending inputs](ending-input-producers.md), not an on-open recomputation or a
fresh population/Base sum. In particular the Clean Reactor rank>=6 condition
means leader/runner-up after an ordinary rank update, not six Bases or turn6.

[Base seeding](startup_base_seed_reference.py) increments95e200 and the owner
summary pass5773c0 clears/rebuilds it. Preserve the retained authority used at
Workshop entry; do not interchange a live count with a stale original cache.

[The independent startup faction initializer](startup_faction_prefix_reference.py)
lines31 onward copies the five AI header values and implements their separate
random-personality/rules mutations. Installed definition offsets are570 fight,
574 wealth,578 power,57c growth and580 tech; native59d1d0 loads those header
integers at59d387..59d3b8. Current copies are the3dc..3ec fields above. Use the
current values if later policy changes/randomization apply, rather than
re-reading immutable header defaults on every open. See
[worker priorities](worker-allocation-priorities.md) for the distinct effective
worker-scoring adaptation.

The MORALE bonus is a different parser branch. `#BONUSNAMES` puts TECH then
MORALE at token buffers96d788/96d7a0. Native59d1d0 initializes definition+4d8
to0 at59d1f6;59d468..59d48f recognizes MORALE, skips an optional `+`, parses the
signed integer and stores+4d8. Its value0 also sets the original separate
morale-impunity flag at+508 bit8000; that flag is not the signed value tested
by the Workshop. Gaia's stored bonus0 is therefore valid; its current AI
fight=-1 is not an alternative value for the reference's `personality` argument.

## Other required cached inputs

The producer reads more than these policy fields. Supply current capability
outputs from577140: own conventional attack95d9bc, own defense95d9c8, own land
speed95d9cc, and peer comparisons95d9d0/95d9d4/95d9d8 as applicable. The existing
[independent capability refresh](startup_capability_reference.py) maps these
without requiring persistent raw20bc storage in the port. PSI comparison
fields stay within that reference's admitted neutral-project scope.

Three additional stored summary fields affect the human sea-transport armor
choice:95d9ec,95f22c and95f230. Do **not** fill them with assumed zeros.
They are cleared and produced by5773c0, not by the smaller577140 refresh:

- `95d9ec` (+844): foreign qualifying ordinary air-attack threat count. The
  unit scan at5779f2..577aaD checks foreign owner/diplomatic admission, air
  domain, raw plan0, nonspecial chassis and592d80 evaluation>=cached peer
  defense95d9d4 before incrementing. It is not simply all visible aircraft.
- `95f22c` (+2084) and `95f230` (+2088): the qualifying own air plan0 and plan4
  counters at578801..578821, within the owner-summary scan's admission
  context. Their complete summary-scan producer remains a separate retained
  authority; this supplement does not reconstruct every admission filter.

Human sea transport chooses the cost99 armor scan only when95d9ec exceeds
**both** own counters; otherwise it uses cost1 armor. A port that lacks these
summary values can still implement the other role families independently,
but must retain this transport decision as an explicit missing input rather
than silently choose the cheap or strongest armor. Raw entry buffers were
explicitly declared in the frozen controls; zero in a fixture was not a rule.

The port can expose a typed input object/accessor for exactly these fields,
technology ownership, catalog records, current capability summaries, per-design
unit/production caches and innate MORALE bonus. The byte buffer in the Python
reference is a convenient address adapter, not a requirement to persist an
opaque native Faction record. Keep unknown required fields visibly unresolved.
