# Stock SMAC Workshop composition families

2026-10-20. Original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This extends [the conventional preview contract](design-workshop-general-visuals.md)
without changing its 15-frame freeze. The new
[source-derived composition reference](workshop_stock_composition_reference.py)
maps the body, weapon, armor and reactor composers to ordered CVR operations.
It consumes installed definition fields; it contains no captured centers.

New private freeze:
`tmp/reference/independent-workshop-stock-families-evidence-20261020/`, manifest
`a080004f9344c4d24341f467f2ce45af951568986c4aebfc7698bbc6b2454219`.
**14 complete indexed/depth frames match independent staged replay: 185 object
projections, 319 raster calls.** Source-derived composition matches 151 ordered
operations across those 14 views and 18 additional composition-only controls.
Four mount vectors are independently decoded from owned marker assets.
No production edits, specialist-checkout access or desktop runs.

The reference is exhaustive for the named stock branch tables; native new-frame
coverage is one view per new chassis/package family, not every cross-product
of weapon, armor, reactor, phase and chassis. Existing active builder records,
initialized Spartan faction material and geometric stage-entry operands remain
declared boundaries, as in the prior renderer contract.

## Operations and caller order

Main composer `4466c0` calls body `4d88e0(owner,ch,we,ar,re,scene)`, weapon
`4d8e40(owner,ch,we,scene)`, armor `4d9130(ch,we,ar,preview=0,scene)`, then
reactor `4d95f0(ch,we,re,abilities,scene)`. Ability decorations follow separately.
The old recorder captured only four reactor arguments; the new recorder also
retains its fifth scene argument. This is an instrumentation correction.

Copy means `632060(model,frame)`; tint-copy means
`6321c0(model,table,frame)`. Attach means `631a60(model,offset,frame)`;
tint-attach means `631be0(model,table,offset,frame)`. Preserve each source
object's frame transform. Do not apply attachment offsets to direct copies.
Unless stated, frame is zero and the model's own shade rows are used.

Owner tint is `78fa98 + owner*6144`, with owner outside 0..7 replaced by zero.
These controls supply the previously independently derived owner-1 Spartan row
at `791298`, hash
`a8416d9624088092b893b4fe4a49214ef6422b0c56748c1f8f1a47cf9e0d74dc`.
Its initialization is required; a zero-filled tint row is invalid input.

Below, **B** abbreviates `VB.CVR`, then owner-tinted `VBP.CVR`;
**G** abbreviates `VGMC.CVR`, then owner-tinted `VGMCP.CVR`.
**P** is `VRCP00..03.cvr`, index reactor-1, omitted for reactor zero.
These are ordered expansions, not interchangeable groups.

## Body: 4d88e0

Body armor class is 1 for IDs 1/2/3/5/6/7, 2 for IDs 4/8, otherwise zero.
Nonzero class suppresses the unarmored placeholder pieces listed below.

| Chassis | Ordered body operations |
| --- | --- |
| 0 Infantry, weapon mode <3 | Attach `VI.CVR` at `(-400,200,0)`, frame phase%15; weapon ID 14 suppresses this branch if definitions are altered |
| 0 Infantry, Probe weapon ID 18 | Attach `VPT.CVR` at the Infantry offset, frame phase%15 |
| 0 Infantry, other noncombat | B, `VCL.cvr`, `VCLT00.CVR`, `VLIGHTS.CVR`; weapon ID 19 instead adds no body |
| 1 Speeder | P, G, B, `VSP.cvr`, `vsptb.cvr` frame phase, `vsptf.cvr` frame phase, `VLIGHTS.CVR` |
| 2 Hovertank | `VB.CVR`, owner-tinted `vht-vbp.cvr`, `vhtp.cvr`, `vhttp.cvr`; if body armor class zero add `vhta00.cvr`, `vhtta00.cvr`; attach `VLIGHTS.CVR` at the Hover-lights marker displacement |
| 3 Foil | P, G, B, `VFL.cvr` |
| 4 Cruiser | B, G, `VCU.cvr`, owner-tinted `VCUP.CVR`, `VCUW.CVR`; add `VCUA00.cvr` only for body armor class zero |
| 5 Needlejet | P, B, G, owner-tinted `VJTP.CVR`; add `VJT00.cvr` only for body armor class zero |
| 6 Copter | P, B, G, `vct.cvr`, owner-tinted `VCTP.CVR`, `VCTB.CVR` frame phase%15; add owner-tinted `vct00.cvr` only for body armor class zero |
| 7 Gravship | P, B, G, `VGS.cvr`, owner-tinted `VGSP.CVR` |
| 8 Missile | `vw13.cvr` for Planet Buster weapon ID 13, otherwise `VM.CVR`; then B |

Every listed operation is a direct copy except the explicitly attached
Infantry/Probe/lights cases. A Foil/Cruiser does not use an Infantry gun offset.

Probe Team on Speeder with armor strength <=1 renders as Infantry. The body
helper applies that remap locally. The main caller `4466c0@4466db..4466f7`
temporarily changes the draft chassis so weapon/armor/reactor also see Infantry,
then restores the original chassis at `4467b2`. Preserve that temporary scope;
this painter does not permanently change the saved design or draft chassis.

### Probe substitutions are scoped to each painter

Do not normalize Speeder Probe chassis globally before constructing all cards.
The chassis card `43de60@43dfd6` calls:

```text
4d88e0(draft.owner, draft.chassis, draft.weapon, 0,
       draft.reactor, Workshop + 1279c)
```

It passes raw chassis/weapon and **forces armor zero**, independent of selected
draft armor. The body helper's local test at `4d8926..4d893d` then remaps
Speeder Probe to Infantry because the installed armor-zero strength is one.
Thus even a draft with Photon Wall selected displays the Infantry Probe body
in its chassis card. It does not write that substituted chassis into the draft.

Shielding card `43e850@43e9cd` calls:

```text
4d9130(draft.chassis, draft.weapon, draft.armor, 1,
       Workshop + 1279c)
```

This also passes raw chassis/weapon, with selected armor and preview one.
`4d9130` does not apply the body helper's Probe remap. For Speeder Probe with
Synthmetal, the standalone shielding card copies `VA01.cvr` and `VSPA01.CVR`,
rather than the Infantry attachment. Armor zero skips the shielding composer
and model draw (`43e999..43e99f` branches to `43ea67`). Armor five retains
the already documented preview-one material behavior.

The reactor card also retains raw chassis/weapon, as pinned in the
[reactor caller clarification](design-workshop-ability-art.md#reactor-side-card-caller-clarification).
Weapon card Probe handling is its separate chassis-minus-one branch, not a
shared remap for other cards. Main assembled model remapping remains scoped
to `4466c0` and its selected armor-strength condition above.

[Six bounded caller controls and two complete helper controls](design-workshop-side-card-probe-cases.json)
verify these arguments and the resulting Probe body / Synthmetal operations.
They supply preceding layout-stage values and final model-copy services;
they add no raster claims. Private additive bundle
`tmp/reference/independent-workshop-side-card-probe-evidence-20261021`, manifest
SHA256 `1efb212ea0fa8dd3259615011fe29b58b109c8a98f06491ea9d00b97a9d9969f`.

## Weapons and packages: 4d8e40

Chassis 8 adds no separate weapon asset: its body already carries the missile.
On other chassis, weapon ID 13 copies `vw13.cvr`, and ID 20 copies `VM.CVR`,
before the ordinary power/mode branches.

For combat weapon modes <3, stock weapon IDs select:

| Weapon ID | CVR |
| --- | --- |
| 0 Hand Weapons | No weapon attachment |
| 1 Laser | `VW01.CVR` |
| 2 Impactor | `VW02.CVR` |
| 3 Gatling | `VW04.CVR` |
| 4 Missile | `VW03.CVR` |
| 5 Chaos | `VW05.CVR` |
| 6 Fusion | `VW06.CVR` |
| 7 Tachyon | `VW07.CVR` |
| 8 Shard | `VW08.CVR` |
| 9 Quantum | `VW09.CVR` |
| 10 Graviton | `VW10.CVR` |
| 11 Singularity | `VW11.CVR` |
| 12 Psi | `VW12.CVR` |

The native selector uses signed weapon **power**, not weapon ID, for this
ordinary mapping. The reference retains its full 0..24 lookup, power clamp,
negative/99 special cases and no-model sentinel. The table above is the
result for installed stock definitions.

For Infantry, attach that asset at the Infantry-gun marker displacement, then
copy `VGMT.CVR` and owner-tinted `VGMTP.CVR`. Hovertank attaches it at the
Hover-gun displacement. Every other non-missile chassis copies it directly.

| Weapon package / mode | Ordered weapon-stage operations |
| --- | --- |
| Troop Transport / 7 | Copy `VWNTT.CVR` |
| Colony / 8 | Infantry: copy `drop.cvr`, then `droplet.cvr`; other chassis: attach `droplet.cvr` at the Colony marker displacement |
| Former / 9 | Infantry: copy `vt.cvr`, then `vwntu.cvr`; other chassis: copy only `vwntu.cvr` |
| Supply / 10 | Copy `VWNST.CVR` |
| Probe / 11 | Infantry: add nothing here, body uses VPT; other chassis: copy `ptmod.cvr` |
| Artifact / 12 | Copy `VWNAA.CVR` |

Thus Former is a noncombat vehicle composition, not VI with a tool attachment.
Its weapon card contains only VT and VWNTU; its full preview also includes
B, VCL, VCLT00, lights and VR00. Colony's marker asset name `adroper.cvr`
does not make that attachment belong to Troop Transport mode 7.

## Armor: 4d9130

Ordinary shell IDs 1/2/3/6/7 share geometry, with tint selection below. ID 5
uses ordinary geometry with cache row 4 only when preview !=0. With preview 0,
ID 5 uses the structural/no-ordinary-shell path and adds no special marker.
Do not turn its card-only tint into an unconditional full-preview shell.

| Armor ID | Material |
| --- | --- |
| 1 Synthmetal | Model's own shades |
| 2 Plasma | Armor cache row 0, `7a7a98` |
| 3 Silksteel | Row 1, `7a9298` |
| 6 Neutronium | Row 2, `7aaa98` |
| 7 Antimatter | Row 3, `7ac298` |
| 5 Probability, preview !=0 | Row 4, `7ada98` |

| Chassis | Ordinary shell operations, using that material |
| --- | --- |
| Infantry combat modes <3 or Probe mode 11 | Attach `vipta00.cvr`, Infantry offset, frame phase%15 |
| Infantry other noncombat; Foil; Gravship; selector chassis -1 | Copy `VA01.cvr` |
| Speeder | Copy `VA01.cvr`, then `VSPA01.CVR` |
| Hovertank | Copy `vhta01.cvr`, then `vhtta01.cvr` |
| Cruiser | Copy `VCUA01.cvr` |
| Needlejet | Copy `VA01.cvr`, then `VJT01.cvr` |
| Copter | Copy `vct01.cvr`, then `VA01.cvr` |
| Missile | No ordinary shell |

Structural IDs 4/8/9 first add respectively `viptawal.cvr` / `viptasgn.cvr` /
`viptapsi.cvr` for combat/Probe Infantry, attached at the Infantry offset with
raw phase as frame argument. For other chassis/packages they copy `a.cvr`
then `apwall.cvr` / `asgen.cvr` / `apsid.cvr`. All use own shades.

Their chassis continuation adds Hovertank `vhta00`+`vhtta00`, Cruiser
`VCUA00`, Needlejet `VJT00`, or Copter `vct00`+`VA01`, as applicable.
ID 5 with preview zero takes the same continuation except Cruiser/Needlejet/
Copter explicitly skip its placeholder; Hovertank still adds both zero shells.
Other chassis add no structural continuation. Preserve these source-order
operations even when they look redundant with body placeholders.

No Armor ID 0 adds no armor-stage objects. Side-card caller preview policy,
Gamma/tint construction and special card raster behavior retain the prior
[component](design-workshop-components.md) and
[palette](design-workshop-picker-backing.md) contracts.

## Reactor: 4d95f0

Reactor zero adds nothing. For installed reactor IDs 1..4, use index reactor-1:

- Combat/Probe Infantry attaches `viptr00..03.cvr` at the Infantry offset.
- Missile copies `vpbr00..03.cvr` directly.
- Every other case, including noncombat Infantry, copies `VR00..03.cvr`.

All reactor-stage frame arguments are zero and materials are own shades.
The general branch additionally copies `VHR.CVR` when ability mask contains
`8000`; that optional decoration is source-mapped but not part of the new
no-ability frames. The caller and reference retain it as a separate condition.

## Mount producers, not captured centers

All four vectors use the same independently decoded owned-asset formula:
`StoreBinary32(frame1.location[j] - frame0.location[j])` for the first object's
two frames. The marker model is temporary. Missing/unloaded model zeros the
vector; a loaded model whose frame count is not 2 leaves the prior vector.

| Offset identity | Marker filename | Global | Subtraction call |
| --- | --- | --- | --- |
| Infantry gun | `AVGMTGUN.CVR` | `7af698` | `45cf41` |
| Colony on non-Infantry | `adroper.cvr` | `7af638` | `45cfbb` |
| Hovertank gun | `AVHT-GunReposition.cvr` | `7af580` | `45d107` |
| Hovertank lights | `AVHT-LightsReposition.cvr` | `7af570` | `45d15a` |

`checked-composition.json` freezes owned asset hashes, decoded locations and
binary32 deltas. Native Hover/Infantry attachment operands match the independently
derived vectors. Colony's non-Infantry attachment is source-traced; its marker
is independently decoded, but the new Colony full preview is Infantry.
The [existing mount/bounds rules](design-workshop-mount-and-bounds-inputs.md)
still require root-geometry bounds, all hidden descriptors in bounds, and
per-draw LOD selection. No captured center enters the new composition reference.

## New controls and remaining limits

Full previews: Infantry Former, Colony, Probe; Foil/Laser/Synthmetal;
Cruiser/Laser/Plasma/Fusion; Needlejet, Hovertank, Copter and Gravship with
Laser/Synthmetal/Fission; Conventional Missile/Fission. Four weapon cards:
Former, Troop Transport, Supply and Artifact. All ability masks and phases are
zero, owner is 1, stored main yaw is zero, x87 control word is `037f`.
Geometry/raster replay consumes declared stage-entry operands and compares
every indexed pixel and depth word. The source-derived composition checker
compares asset, copy/attachment operation, material, frame, offset and order.

The 18 composition-only controls exercise all nine body chassis at Laser/
Synthmetal/Fission and all weapon IDs 12..20 on Infantry. They do not paint
another matrix. No prior conventional raster or commit matrix was rerun.

No additional kernel was needed for these 14 frames; the established ordinary/
clipped Single and Square2 implementation replays all 319 recorded calls.
This is evidence for these compositions, not blanket coverage of every model,
animation frame, ability decoration or translucent painter.

Animation remains a separate verification boundary. The source passes phase
or phase%15 as listed above. Copter also calls `610d10` on the Workshop object
at `4d8d89..4d8d97` and increments global phase when that predicate succeeds.
The new controls do not establish timer scheduling, active-window behavior,
nonzero-frame motion or frame rollover. They must not be used as permission
to freeze animated models at frame zero. SMAX models remain outside this pass.
The later [stock ability artwork contract](design-workshop-ability-art.md)
establishes ten decorator bits, the extra Artillery reactor model, main/strip
ordering and 32 independently checked full indexed/depth views.

The [animation-controller handoff](design-workshop-animation-controller.md)
now establishes timer delivery, hierarchy visibility, paint-dependent phase
and yaw advancement, and the precise nonzero-frame raster checks still needed.

Private opcode/address disassembly covers `4466c0`, all four composers,
`45cb80`, all four copy/attachment methods, `641f10`, main/weapon painters and
draw dispatch. `source-jump-tables.json` decodes body chassis `4d8e1c`, body
armor class `4d8df4`, armor material `4d959c`, armor chassis+1 `4d95c4`, and
package mode-7 `4d9118`. `stock-model-map.json` maps original loaded model
pointers to owned filenames and file hashes, including otherwise easy-to-miss
dynamic `viptr`, `VRCP`, `VR` and `vpbr` arrays.
