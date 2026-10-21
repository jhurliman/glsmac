# Conventional land Workshop construction and editing

2026-10-20. Original terran.exe, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
No production edits or desktop interaction. Additive private evidence:
`tmp/reference/independent-workshop-general-evidence-20261020/`, manifest
`f8c5706cb48dfbab7581793f1f4e1371c4ec0c1ff1e17cddfc7262f860b480db`.

The [independent reference](workshop_general_reference.py) matches **40 complete
26,624-byte design pools and automatic names, 216 compatibility predicates,
11 complete draft cleanup regions, and seven complete saved-design commits**.
The latter also pass original production eligibility `5d1eb0(index,1,-1)`.
All controls use original installed rules/text; native results are expectations,
never algorithm inputs. Cost uses the established shared cost reference.

## Live edit sequence

Keep draft state separate from the saved 52-byte design record. Draft fields
relative to the Workshop object: owner `141f4`, selected ID `141f8`, chassis
`141fc`, weapon `14200`, armor `14204`, ability mask `14208`, reactor `1420c`,
ability selector indices `14218/1421c`, name `14220`.

Component picker admission, draft compatibility cleanup, record construction,
automatic naming, and saved-design allocation are distinct operations:

1. Filter configured definitions by prerequisite >= -1; use actual owner
   technology to admit selection. Keep unknown admitted definitions visible
   according to the existing silhouette presentation contract.
2. Update the component and run the caller's automatic-ability suggestions,
   then `443d20` cleanup. Reactors update their rating and run cleanup.
3. Apply/Done follows `441e20`: exact-tuple reuse or allocation, original
   builder `5bd4f0`, automatic naming `594320`, saved-name handling, history
   propagation and existing refresh/close behavior.

Use the existing [occupied-bank commit contract](design-workshop-custom-edit.md)
for allocation, overwrites, explicit names, retirement, cached counts and queue
effects. This supplement generalizes the constructed tuple beyond Scout/Synth;
it does not replace those already verified commit rules.

Ordinary Infantry/Rover switches retain conventional weapon and armor when
their domain is unchanged, then clean incompatible abilities. Source-only
caller trace: chassis `4440b6..444201`, weapon `4444d0..44465d`, armor
`444932..444969`, reactor `444c23..444c30`. The seven complete commits below
write draft inputs directly, so they do not claim mouse/modal execution of
these suggestion branches.

Automatic suggestion helper `443e50` first cleans the existing mask. It adds
the supplied ability only when setting `959f98 & 4000` is enabled, the owner
has its actual prerequisite, `59bd40` permits it, at most one ability already
remains, and Neural Grafting is owned if one remains. It then rebuilds selector
indices ascending. Weapon-change suggestions on their admitted branch include
Heavy Transport for transport mode 7, Super Former then Fungicide for former
mode 9, and Blink when offense >= twice defense. An armor upgrade above 1 with
weapon offense <=1 suggests Comm Jammer. These are suggestions, not unconditional
properties of a new design. Source guards and automatic-setting gate are frozen.

## Ability compatibility: 59bd40

The predicate accepts ability index, chassis, weapon and armor. It does not
check owner technology; that belongs to picker/suggestion admission.
Ability definition flags have these exact tests:

| Flag | Test |
| --- | --- |
| 1 / 2 / 4 | Must admit chassis land / sea / air domain |
| 8 | Required for any nonzero offense, including negative psi offense |
| 16 | Required for zero-offense former mode 9 |
| 32 | Required for other zero-offense packages |
| 64 | Reject probe mode 11 |
| 128 | Reject negative weapon offense or negative armor defense |
| 256 | Require transport mode 7 |
| 512 | Reject chassis base speed >1 |

Any nonzero chassis special/missile byte rejects all abilities. The ability
land-cost flag 1024 belongs to cost calculation, not these compatibility tests.
The 216 controls test all 24 abilities across Infantry Hand/No Armor,
Infantry Laser/Plasma, Rover Laser/Synth, former, probe, transport, psi,
Foil and Needlejet contexts. Conventional land is the construction/naming scope;
the last contexts are predicate controls only.

Cleanup `443d20` visits bits 0..23 ascending. It removes incompatible bits,
Carrier bit 7 if Deep Pressure bit 6 is also selected, the current bit when
more than two are selected, and the current bit when two are selected without
the second-ability technology at `9ba470` (Neural Grafting). Consequently an
overfull valid mask retains the **highest two bits**, or highest one without
that technology. It rebuilds selector indices after each removal. If no removal
occurs, it preserves existing selector values. Special chassis clears the mask
and both selectors. Cleanup does not recheck every selected ability prerequisite:
the disabled Heavy Transport control remains compatibility-valid there even
though the picker must not offer its disabled definition.

## Record construction: 5bd4f0

Arguments are `(index,chassis,weapon,armor,abilityMask,reactor)`. This routine
does not validate technology or component legality. It writes tuple bytes
32..39, transport capacity 40, shared raw cost low byte 41, plan 42, category
43, zero owner-exclusion byte 44, owner claim 45, icon 46=255 and flags word 48.
It preserves all 32 name bytes, byte 47 and prerequisite word 50. Inputs poison
those preserved fields and compare the entire design pool afterward.

Transport capacity starts at chassis rule capacity; sea multiplies by reactor,
Slow halves it, Heavy Transport applies `(3*capacity+1)/2` integer truncation.
Other modes store zero capacity. Existing shared cost/prototype formulas remain
authoritative; this work does not create another price formula.

For ordinary land combat, let O be signed offense, D signed defense and S base
chassis speed. Plan starts 0, becomes 2 if D>1, then 1 if O>=D. If S>1 and
O>D it becomes 0. Amphibious/Drop/Artillery mask `8018` forces plan 0.
Category starts 3, becomes 2 if D>O or 1 if O>2D; S>=2 overrides to 4 and S>=3
to 5. When O=D=1, category is 10; S>1 sets plan 3, then Non-Lethal Methods
sets plan 2. Plan and category are separate bytes.

Package mode >=7 uses plan=mode, category=mode+32. Sea combat uses 6/6;
air uses 4/9 with Air Superiority, otherwise 0/8. Special missile chassis
uses 5/14 for O>=99, otherwise 0/13. These branches are source traced; the
40 builder controls concentrate on conventional land and ordinary packages.

Prototype coverage scans active technology-admitted built-ins and active own
prototyped designs. Weapon/armor are covered by exact ID or an independently
stronger positive component; chassis by exact ID or same-domain speed >= target.
Components can be covered by different designs. Reactor and abilities are not
examined. Covered records receive flags `105`, uncovered `1`. Native quirk:
if covered and the **last own slot's old flags** contain `10`, flags are `115`,
even when that last slot is inactive. The scan precedes record writes.

## Automatic names: 594320

The reference uses resolved original language fields as declared inputs, not
native output names. Its supported context is post-build conventional land
and ordinary land packages; arbitrary preview contexts, psi naming, and other
domains are outside the 40 name controls.

Two language variants are assembled, with initial choice
`((reactor-1)^owner)&1` and offense/defense parity adjustments. Ordinary offensive
names use weapon short text plus chassis name/ordinary text; defense-dominant
names use armor short text plus defense alternatives. The low-strength branch
uses Scout/Recon wording; sufficiently strong components use the chassis elite
forms. Artillery uses Artillery/Battery suffixes; offensive Infantry Amphibious
uses Marines/Invaders. Faster land packages may include reactor and chassis text.
See the executable reference for exact branch order and thresholds.

Only the first selected nonempty ability short prefix in ascending index order
is used. Artillery is handled as a suffix. Offensive Infantry Amphibious can
also suppress its prefix in favor of the suffix. Multiple abilities do not imply
multiple name prefixes. Preserve the measured native quirk **`Polic Rover`**.

Collision scans include built-ins and the owner's bank **even for inactive
records**, skip the current ID and identical component tuples, and ignore foreign
banks. A collision switches the chosen variant and increments the conflicting
variant with localized Mk2/Mk3 etc, restarting the scan. Controls cover both
variants, inactive records, exact tuples, foreign records and Mk3; arbitrary
long chains/localization beyond this bounded reference are not claimed.
The native function can emit more than 31 bytes. The commit copies only 31
bytes plus NUL, preserving the remainder of the stored name field.

## Representative complete commits

Each starts with selected built-in Scout, an empty own bank, a separate foreign
design, three bases and no live units. Native `441e20` selects slot 127. All
26,624 design bytes match the independent builder/name/commit prediction;
faction, base, unit and history buffers remain unchanged. Every saved design is
production-eligible through native `5d1eb0`. Full 52-byte records are in frozen
`checked-contract.json`, with input/output pools in `native-observations.json.z`.

| Draft | Stored automatic name |
| --- | --- |
| Infantry, Laser, Synthmetal, Fission | Laser Squad |
| Rover, Impact, Synthmetal, Fission | Impact Rover |
| Infantry, Laser, Plasma, Fusion | Plasma Garrison |
| Infantry, Laser, Synthmetal, Fission, Clean | Clean Laser Squad |
| Infantry, Impact, Plasma, Fission, Artillery | Impact Battery |
| Infantry, Laser, Synthmetal, Fusion, Clean+Trance | Clean Laser Infantry |
| Infantry, Singularity Laser, Antimatter, Singularity, Amphibious+Artillery | Amphibious Singularity Artiller |

The final row deliberately proves the 31-byte save boundary. Builder controls
also cover all four reactors, Rover Quantum/Singularity, defensive armor,
Trained, Police, Drop, AAA+Jammer, independent component history coverage,
Super Former, Secure Probe, Clean Supply and Heavy Transport. Existing occupied
bank controls cover how these new tuples connect to saved designs and queues;
the seven new controls prove the generalized construction enters that path.
