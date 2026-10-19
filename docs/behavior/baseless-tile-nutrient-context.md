# Base-less tile nutrient context for improvement art

The developed-overlay caller evaluates nutrients with `(faction,-1,x,y,0)`.
`base=-1` is an explicit context, not a nearby-Base lookup. Reuse the ordinary
nutrient engine with these bindings, without requiring a Base object.
[Cases](baseless-tile-nutrient-context-cases.json) contain20 complete original
queries and9 native caller faction-selection controls.

| Engine input | Native base-less binding |
| --- | --- |
| Terrain/items/resources | Current live tile, even when remembered items admit the sprite |
| Tree Farm / Hybrid Forest | **False explicitly:** negative Base ID skips both facility queries |
| Bumper Crop / Famine | **False explicitly:** negative Base ID skips the Base event-state read and both adjustments |
| Nutrient-limit technology | Configured technology eligibility/ownership for the resolved evaluating faction |
| Fungal PLANET | Evaluating faction's **current effective PLANET**, not retained turn snapshot |
| Fungal capability | Evaluating faction's already-built nutrient capability |
| Fungus bypass mode | 0, enabling normal fungal processing |

Correction to the earlier faction-fallback prose: a **nonnegative renderer
viewer is used unchanged**. Viewer0 remains0. Only a negative renderer viewer
resolves through the live tile byte5 low three bits, then through the global
viewer if those bits are0. A visible foreign tile is therefore still evaluated
for the supplied nonnegative viewer. The nutrient function takes that faction
argument directly and performs no additional fallback. Its technology helper
returns false for faction0, even for configured technology ID -1.

The fungal query reads current PLANET and capability directly. It adds
`clamp(currentPLANET,-3,0)` to capability, then clamps to0..99. Changing only
retained PLANET has no effect. Reuse the existing
[capability construction](fungal-resource-capabilities.md); the query does not
rebuild social ratings or fungal capability. The ordinary technology cap still
reads live technology records and ownership, independently of any Base.

`base=-1` **does not force a non-center calculation**. The live tile takes the
center branch if live item bit1 is set and tile byte2 low nibble is0..7. An absent
live bit1 or nibble8..15 takes the ordinary branch. This test does not resolve a
real Base ID and does not use remembered center bits. On the center branch use
the center Nutrient rule, double it for a resolved nutrient resource, and apply
the existing applicable landmark point. Negative Base ID skips center facility
effects too. Never borrow Recycling/Pressure-Dome effects from a nearby or
globally focused Base.

The tests poison Base0's complete facility bitmap and event state. Base-less
Forest yields1, versus3 for Base0 with Tree Farm+Hybrid; Bumper raises Base0 to4
and Famine lowers it to2 while the base-less result stays1. Base-less center
yields2 versus Base0's facility-boosted3. Every negative-Base control performs
**zero Base-record reads and zero facility-helper calls**. Contrasting current
and retained PLANET gives fungal yields3 versus5 from capability5. Rainy Farm
still yields2 before the cap technology and3 after it.

Native removed-cap excess accumulates in a scratch counter. The art adapter
consumes the nutrient result without posting this diagnostic excess into a real
Base's intake. These controls bind context; they do not replace the
[ordinary arithmetic rules](ordinary-tile-nutrients.md).
