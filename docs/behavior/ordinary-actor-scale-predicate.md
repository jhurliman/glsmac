# Ordinary actor scale: stored component predicate

The ordinary idle scale multiplier is **not simply chassis0**, nor is it a
test for combat weapon mode or the resolved Infantry body. Original4d7860
stores its base scale through6324f0 at4d7bfd, then calls4d8400 at4d7c1c.
The entry block **4d8420..4d8478** reads the unit's design ID and that design's
stored chassis and weapon **IDs**:

| Stored component IDs | Entry multiplier |
| --- | --- |
| chassis0, weapon0..2 (Gun/Laser/Impact) | 1.25 |
| chassis0, weapon18 (Probe) | 1.25 |
| chassis0, any other weapon | binary32 0.8 (`0x3f4ccccd`) |
| chassis1, weapon18 (Probe), any armor | 1.25 |
| other chassis/weapon combinations | 1.0; no scale setter here |

The1.25 constant is binary32 at68fcb0. The0.8 constant at68fcb4 is exactly
`0.800000011920929`. Both modified branches read the already stored scene
scale via632500, multiply by the constant and store through6324f0. Keep this
separate from the ordinary base-scale calculation and its binary32 store.
The symbolic predicate is provided in
[ordinary_actor_scale_reference.py](ordinary_actor_scale_reference.py).

This makes chassis0 Former15 and Colony Pod14 use0.8. It also makes chassis0
Gatling3, Missile4 and PSI12 use0.8 despite their combat modes. Chassis0
Gun/Laser/Impact/Probe use1.25. These examples are consequences of the source
branch, not additional claimed native draw controls.

The Probe body remap happens **later**, at4d8505..4d8530. It changes local
chassis1 to0 for weapon18 only when signed armor strength<=1. The multiplier
has already been selected and does not read armor. A Speeder Probe with
Synthmetal therefore still gets1.25 even though this later body remap does not
occur. Do not derive the multiplier from the final body/composition chassis.

There are additional later modifiers for raw native design IDs8,9,10, starting
at4d84a8 and handled at4d869f/4d8647/4d85f9. The entry predicate above is not
a replacement for those native-creature branches.

The four-family [strip raster handoff](design-workshop-strip-raster.md) remains
unchanged: its Scout is chassis0/weapon0; Rover, Foil and Needlejet are outside
the multiplied combinations. Its shorthand about Infantry scale should be
read using this exact predicate when extending to other designs.

This is a **static source clarification**, with no new model draws or matrix.
The branch is unambiguous and independently distinguishes noncombat weapons
and both armored/unarmored Speeder Probes. Additive source freeze:
`tmp/reference/independent-ordinary-scale-predicate-evidence-20261021/`.
