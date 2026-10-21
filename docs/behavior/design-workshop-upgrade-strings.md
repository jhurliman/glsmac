# Exact stored-design strings in Upgrade prompts

2026-10-21, pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Eleven bounded original5935c0 strings, three original504fe0 prompt calls,
and source traces; no layout matrix, campaign or conversion runs.
[Reference](workshop_upgrade_string_reference.py),
[scalar receipts](design-workshop-upgrade-strings-cases.json).

All11 stat strings,3 prompt pairs,24 ability tokens and10 source literals
pass independent checks. Additive private freeze:
`tmp/reference/independent-workshop-upgrade-strings-evidence-20261021/`,
10 payloads, manifest
`dc149d333eb0ec135bd592476b2a4c259dd1c4ecd437a685dad43cadbc05dd95`.
This provenance paragraph was added after freezing.

## Complete UNITTYPE0/1 grammar

`5935c0(dst,storedDesignID)` **appends** a stats string. It does not supply
the name or the outer parentheses. `504fe0@505166..50548f` builds each
parameter separately from the stored source and target records:

```
storedName + " (" + storedStats
  + (rawAbilityMask != 0 ? " " + abilityTokensInBitOrder : "")
  + ")"
```

UNITTYPE0 is the selected stored source; UNITTYPE1 is the stored destination.
Do not substitute unsaved Workshop scratch design63 or the generated title.
The resolved strings are handed to `633910` indices0 and1.

The loop reads ability definition **first-field tokens** at
`9b6c70 + 28*slot`, resolved through `62e860`. These are short modifiers,
not the full display names, and not the DATA panel's one-character shorthand.
It visits slots0..23 in ascending order and appends each string **directly**.
There is one space before the entire suffix and **no spaces, commas or
separators added between individual ability tokens**.

| Slot | Bit (hex) | Exact stock token |
| --- | --- | --- |
| 0 | 1 | Super |
| 1 | 2 | empty string |
| 2 | 4 | Cloaked |
| 3 | 8 | Amphibious |
| 4 | 10 | Drop |
| 5 | 20 | SAM |
| 6 | 40 | Sub |
| 7 | 80 | Carrier |
| 8 | 100 | AAA |
| 9 | 200 | ECM |
| 10 | 400 | Grav |
| 11 | 800 | Empath |
| 12 | 1000 | Secure |
| 13 | 2000 | Fungicidal |
| 14 | 4000 | Trained |
| 15 | 8000 | Artillery |
| 16 | 10000 | Clean |
| 17 | 20000 | Blink |
| 18 | 40000 | Trance |
| 19 | 80000 | Heavy |
| 20 | 100000 | X |
| 21 | 200000 | Repair |
| 22 | 400000 | Police |
| 23 | 800000 | Slow |

The preliminary nonzero test uses the whole raw32-bit mask, but the loop
stops after24 slots. Bit1 alone or bit31 alone therefore produces a trailing
space before the closing parenthesis, even though no token is appended.
That behavior was executed, not inferred from an empty label.

Exact original504fe0 receipts:

```
Upgrade Source (1^-2-1 CloakedDropCarrier)
Upgrade Target 65 (2~-3+t-1 AmphibiousECMTrance)
Upgrade Source (1-2-1*2)
Upgrade Target 65 (2-3-1*2)
Upgrade Source (1-2-1 )
Upgrade Target 65 (2-3-1 )
```

These prompt calls stop with supplied dialog return0, so no upgrade executes.

## Stats formatter5935c0

This reuses the [main stat arithmetic](design-workshop-main-presentation.md)
with two presentation differences: negative attack/defense prints localized
**Psi**, not `?`; and sea/air gets a domain word before the reactor suffix.

```
attack-defense-displayedMovement[ " Sea" | " Air" ][ "*" + reactorRating ]
```

Land has no domain word. LABELS byte offsets310,28c,288 resolve Psi,Sea,Air.
The reactor suffix is present only when the stored unsigned reactor byte>1.
It prints the **rating**2/3/4, not the power20/30/40; it follows the domain
word, so native Fusion Foil is `(1)-1-4 Sea*2`, and Fusion Needlejet is
`1-1-12 Air*2`. The stat formatter itself adds no final `)` beyond an
attack-specific artillery/sea pair.

Attack uses signed raw weapon strength: negative→Psi, >=99→`*`, both skip
attack decorations. Ordinary attack uses592f10 /5d91c0(id,-1,0), including
the chassis special multiplier trunc(3*offense/2). Zero remains0. It appends
ability symbols in this order: bit10 `^`, bit8 `~`, bit100000 `x`, bit800 `e`,
bit20000 `!`. Offense>0 and armor>=0 permits parentheses for sea or land
artillery8000. Ability20 wraps the resulting decorated attack in `<...>`.

Defense negative→Psi and skips decorations. Otherwise use
5d92f0(id,-1,0), which clamps strength to at least1. Bit100 adds `<` before
the number; bit200 appends `+`; bit40000 appends `t`; bit100 closes `>`.
Combined scalar receipt: `<1^~xe!>-<1+t>-1*2`. Double PSI with the same mask
is `Psi-Psi-1*2`; a special weapon retains `*-<1+t>-1`.

Movement is signed truncation of native5d9400(id)/movementQuantum9ba428.
Reuse its chassis/reactor/transport/Slow/Antigrav/Air Superiority rules;
do not multiply the displayed movement by reactor power a second time.

## Main Workshop Upgrade action7

At the existing1024 profile, parent-local rectangle is
**`[351,347,456,367)`**,105x20. Child is Workshop+5c00; rect storage is
Workshop+bbc. Constructor `4469a0@447484..447505` resolves LABELS byte
offset **0xbd4**, exact stock text **UPGRADE**, then calls621620 with action7.
0xbd4/4=757 zero-based token, TXTLoader row758, or array index758 when the
section count occupies index0. Tooltip uses offset0xc5c.
This is the normal Workshop action button's existing style27 path;
the economic modal OK sprite correction is a separate control.
