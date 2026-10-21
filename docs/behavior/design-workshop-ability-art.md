# Workshop stock ability model composition

Stock abilities do change the assembled Workshop model. Ten bits append owned
CVR artwork through `4d8700`; Heavy Artillery also changes the general reactor
composition. Both the main preview and ordinary saved-strip CVR path use these
models. This is an additional rendering slice beyond the stock body, weapon,
armor and reactor families.

Authority: original GOG `terran.exe`, SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The [scalar/hash receipts](design-workshop-ability-art-cases.json) and
[self-authored composition reference](workshop_ability_art_reference.py)
accompany this contract. Owned geometry, indexed surfaces, depth surfaces,
palette lookups and disassembly remain in ignored private evidence.

## Decorator branches and attachment policy

`4d8700(mask,scene,owner)` reads the mask and scene; the owner argument is
unused. The following is **source execution order**, which differs from bit
order. Preserve it when multiple abilities are present.

| Bit | Mask | Stock label | Owned asset | Model pointer | Guard |
| --- | --- | --- | --- | --- | --- |
| 0 | `000001` | Super Former | `ssf.cvr` | `7af930` | `4d870a` |
| 13 | `002000` | Fungicide Tanks | `sft.cvr` | `7af8d0` | `4d871d` |
| 5 | `000020` | Air Superiority | `sas.cvr` | `7af910` | `4d8730` |
| 7 | `000080` | Carrier Deck | `scd.cvr` | `7af900` | `4d8743` |
| 10 | `000400` | Antigrav Struts | `sags.cvr` | `7af8e0` | `4d8756` |
| 15 | `008000` | Heavy Artillery | `vhr.cvr` | `7af8c0` | `4d8769` |
| 21 | `200000` | Repair Bay | `srb.cvr` | `7af8a0` | `4d877c` |
| 4 | `000010` | Drop Pods | `sdp.cvr` | `7af920` | `4d8792` |
| 9 | `000200` | Comm Jammer | `scj.cvr` | `7af8f0` | `4d87a5` |
| 18 | `040000` | Hypnotic Trance | `sht.cvr` | `7af8b0` | `4d87b8` |

Every branch calls `scene.632060(model,0)`. This is direct frame copying:
retain the asset's own frame-zero locations and matrices and its own shade
tables. There is no attachment offset, faction recolor, armor recolor,
ability-specific rotation or chassis-specific placement in this helper.
Do not replace the decoded transforms with identity transforms. The normal
scene/camera transforms still apply after assembly.

All ten installed assets have one animation frame and real geometry. Drop
Pods contains two objects; each other asset contains one. Their frame-zero
visibility flags are zero. Ability art therefore stays at frame zero even
when the body has an animated frame. Body animation and main yaw continue
according to the existing controller contract.

The remaining stock bits append no model through this helper:
1 Deep Radar, 2 Cloaking Device, 3 Amphibious Pods, 6 Deep Pressure Hull,
8 AAA Tracking, 11 Empath Song, 12 Polymorphic Encryption, 14 High Morale,
16 Clean Reactor, 17 Blink Displacer, 19 Heavy Transport, 20 Nerve Gas Pods,
22 Non-Lethal Methods and 23 Slow Unit. The installed prerequisites disable
19 and 23; their raw-bit helper controls are diagnostic. This finding concerns
model composition, not those abilities' gameplay effects or other UI labels.

## Main and strip ordering

Main `4466c0` builds body (`4d88e0`, call `446725`), weapon (`4d8e40`,
`44674a`), armor (`4d9130`, `446771`), reactor (`4d95f0`, `446793`), then
ability decorations (`4d8700`, `4467aa`). Global `7528f8` is one only around
the weapon call, as in the existing main composition contract.

Ordinary idle strip `4d8400` builds body at `4d8538`, weapon at `4d8541`,
ability decorations at `4d854d`, armor at `4d855c`, then its inline reactor
stage. Do not reuse the main list unchanged for the strip. Equal-depth
overlap is sensitive to traversal order because the renderer uses a strict
depth comparison.

The cached sprite branch still bypasses CVR construction for ordinary selected
rows at or above nine. See the [cached preview contract](design-workshop-cached-preview.md)
for its guard and reserved-row alias edge. A selected ability does not force
that branch to switch to a CVR composition.

## Heavy Artillery reactor edge

The main reactor helper has five arguments:
`4d95f0(chassis,weapon,reactor,abilities,scene)`.

- Reactor zero exits without adding a reactor model.
- Infantry combat modes below three and Probe mode eleven attach
  `viptrXX.cvr` at `(-400,200,0)` and return before the artillery test.
- Missile chassis eight copies `vpbrXX.cvr` and returns before that test.
- Other cases copy `VRXX.cvr`, then copy `VHR.CVR` at frame zero when
  `abilities & 8000` is nonzero (`4d9672`, call `4d9680`).

The strip repeats that final test inline at `4d85d9`, with its VHR copy at
`4d85ee`. The reactor's VHR pointer is `7af708`, distinct from the decorator's
`7af8c0`; both load the same Windows file. Preserve both copies.

Consequently, legal Rover and Hovertank artillery compositions contain two VHR
copies. Main order is reactor VR, reactor VHR, decorator VHR. Strip order is
decorator VHR, armor, reactor VR, reactor VHR. Legal Infantry artillery has
only the decorator VHR. Duplicate geometry may be depth-redundant; this is
not permission to deduplicate the original operations.

Stock Heavy Artillery compatibility flags are `0089`: land combat, excluding
negative weapon/armor ratings. Thus the general-reactor artillery branch does
not establish legal sea artillery, Former artillery or missile artillery.
Those supplied-mask fragment cases are explicitly diagnostic. The same
composer can execute on a raw record that the normal ability picker rejects.

## Reachability and visible effects

### Reactor side card caller clarification

The standalone reactor card painter is `43ed10`; `43e2b0` paints the weapon
card. At `43ef7d` the reactor card calls:

```text
4d95f0(draft.chassis, draft.weapon, draft.reactor,
       draft.abilities, Workshop + 1279c)
```

The five argument producers are draft fields `141fc`, `14200`, `1420c`,
`14208`, and the scene address respectively. The reactor value is loaded at
`43ef52`; weapon at `43ef62`; abilities at `43ef6c`; chassis at `43ef74`.
There is no zero-mask substitution and no chassis/weapon substitution in this
painter. It does not call the separate decorator `4d8700`.

Thus a Rover/Hovertank artillery reactor card contains `VRXX` followed by
**one** reactor-stage VHR copy. Infantry combat artillery uses only its
`viptrXX` attachment in this card; its decorator VHR belongs to the assembled
main/strip model, not this standalone card. Speeder Probe likewise retains
Speeder chassis here and uses the general `VRXX` branch. The temporary
Speeder-Probe-to-Infantry remap is scoped to main composer `4466c0`, which runs
later in a full refresh (`441d19` reactor card, `441d34` main painter).
The small wrapper `446870` also forwards all four draft values unchanged.

Five bounded original call-prefix controls verify the actual five arguments,
including Artillery on Infantry/Rover/Hovertank, no-ability Rover, and Speeder
Probe. See [caller receipts](design-workshop-reactor-card-ability-cases.json).
Private additive bundle:
`tmp/reference/independent-workshop-reactor-card-ability-evidence-20261021`,
manifest SHA256
`5faafb676bbbce2afda431e58fdf0420deaeeb662edb1c870080ad5efad41804`.
These controls supply preceding text-layout results and stop at the composer
entry; they do not claim new card rasters. The earlier ability-art freeze is
unchanged.

### Full model fixtures

The raster fixtures execute original compatibility `59bd40` and prerequisite
ownership `5d17b0` before building each nonzero ability recipe. Every sampled
bit passes both. Owner one has the required technologies in this declared
fixture; this does not assert that the abilities are available at game start.
Carrier Deck and Repair Bay use Foil/Troop Transport because stock admission
requires a transport package. Super Former and Fungicide use a Former.
The other single-bit recipes use Infantry/Hand Weapons, except the additional
Rover artillery case. A legal SAM plus Drop Pods recipe checks the non-bit-order
decorator sequence. Fission and no armor are common to all cases.

Each recipe has main and strip captures. Four same-run zero-ability baselines
cover Former, Foil Transport, Rover and Scout. Every nonzero recipe changes
main indexed color and depth relative to its matching baseline. Every sampled
strip recipe also changes both, except Comm Jammer: its added object changes
five indexed/depth cells in the main preview and zero in this strip view.
Keep the object even when hidden by the sampled angle, scale or other geometry.
The receipts report all differences rather than assuming appended geometry
must produce visible pixels at every scale.

## Evidence and verification boundary

All 32 fixtures use the **same supplied Spartan owner-one material** as the
earlier stock-composition receipts: read owned `spartans.pcx` at `(435,745)`,
loaded tint index 80, RGB `(64,76,76)`. Derive the 6,144-byte owner table from
that tint, `VBP.CVR`'s 24-by-256 shade rows and the active Gamma100 palette,
following the [established material contract](design-workshop-general-visuals.md).
The destination is `791298`; expected table SHA256 is
`a8416d9624088092b893b4fe4a49214ef6422b0c56748c1f8f1a47cf9e0d74dc`.
The frozen `fixture-inputs.json.body_material` declares the source, sample and
input hashes. Derive the table from those owned inputs; the hash is a check,
not a captured table to use as the algorithm. This faction tint applies only
to the existing owner-tinted model operations; the ability additions retain
their own shades.

Private immutable bundle:
`tmp/reference/independent-workshop-ability-art-evidence-20261021`.
Manifest SHA256:
`54e26b4509e78d63cdbcc2eb4d8777a82fa00def8e263183e0f046d8a880518c`.

The 399 payloads include 32 full indexed/depth captures, 24 nonzero-ability
views and eight baselines. Source-derived operation order, material, frame
and attachment policy match all 1,190 assembled objects. The checker decodes
owned CVR frames independently and recomputes all 16 main bounds and centers.
The staged projection/raster replay matches every color byte and depth word
across 1,548 kernel calls. Fifty complete decorator controls cover every stock
bit with owners one and seven, plus two all-bit order diagnostics; fourteen
reactor controls isolate its early returns and extra VHR branch.

The raster replay consumes recorded native kernel-entry streams, origins,
bases and lookups. It is not an independent asset-to-kernel renderer. Strip
camera/placement enters at the recorded prepass boundary; main camera and
faction materials retain the declared initialized-stage boundary from earlier
receipts. Strip captures prove the internal 256-square model surface, not the
complete final console canvas. The shadow lookup is initialized under `027f`
and rendering uses `037f`, separately recorded; see the
[precision-lifetime contract](design-workshop-shadow-precision-lifetime.md).

The older observer records only four outer reactor arguments; the actual
five-argument signature is source-pinned, and all child model operations and
full outputs are captured. This metadata limitation does not supply expected
composition or raster output to either checker.

These controls close the stock ability model-selection/assembly gap for the
listed branches. They do not exhaust all chassis, armor, ability pairs, yaw
angles or SMAX assets. No production files or checkout references were changed.
