# Ordinary map object anchors

> Semantic correction, revision2: original improvement names were wrong. Use the corrected raw-mask table and contexts in [the correction handoff](ordinary-map-improvement-semantic-correction.md). Frozen v1 coordinate evidence is retained; its old feature labels are superseded. Base/Scout rules are unchanged.


Pinned stock SMAC; actual ordinary map iteration and its object consumers were executed in a controlled1024×511 map viewport, camera(40,40), tile(40,40), zoom0, round80×80 world. Adjacent cases contain48 assertions, four native Base/name/Scout fixtures, four native model surface setups, and20 admitted improvement sprite calls. No production code changed.

Let F=(576,288) be this fixture's flat diamond center, H=floor((W+N+E+S)/4), and P=(F.x−48,F.y−24−H), the original projector result. The actual ordinary map caller supplies **O=(P.x,P.y−24)** to Base and Unit dispatch. The independent Base repaint pass makes the same subtraction. These offsets belong to zoom0; other zooms were not generalized here.

| Fixture | C/W/N/E/S heights | H | P | Base/Scout call O | Scout model surface | Base name baseline |
|---|---|---|---|---|---|---|
|Uniform band3|0/0/0/0/0|0|(528,264)|(528,240)|(578,290)|315|
|Uniform band4|25/25/25/25/25|25|(528,239)|(528,215)|(578,265)|290|
|Single hill|25/6/6/6/6|6|(528,258)|(528,234)|(578,284)|309|
|Hill at coast|25/0/0/6/6|3|(528,261)|(528,237)|(578,287)|312|

Single hill is band4 at(40,40) in a band3 world. Coast additionally makes(39,39) band2. Native cached corner-height construction and native projection execute after each world reset. The single hill and coast distinguish the outer-height average from the25-pixel fan center.

The actual Base artwork call receives O; its sprite metadata supplies further rectangle offsets. Population3 uses outer box[O.x,O.y,O.x+14,O.y+16), text start O.x+3 and baseline O.y+14. This fixture has a valid Unit marker, so the population box is faction-filled. Base-name rendering independently projects the Base: `New Austin` foreground starts at(P.x+17,P.y+51 baseline), with shadow(+1,+1). It does not reuse O as its label anchor. Existing Base-name font and overlay contracts still specify colors/metrics.

The native predefined Scout Patrol design2, Hold order, damage3 and singleton stack reaches the conventional model renderer with O, flags3,zoom0,context1. Replaying those actual arguments through the original model caller/composition reaches native surface setup at **O+(50,50)**. Conventional infantry/weapon mode0 bypasses that caller's optional chassis/weapon slope branch. This closes the model's surface anchor; a full model ink rectangle is not asserted.

The same actual Unit draw paints black health[O.x+6,O.y+6,O.x+8,O.y+27), then damage3 colored fill[O.x+6,O.y+6,O.x+8,O.y+20). Native Hold text starts O.x+11 with baseline O.y+16. The four cases include final coordinates, not only relative formulas. Body, badge, health and order share O before their individual renderer offsets.

Bunker, airbase, mine, solar and condenser sprite callers instead receive **P**, without the object's extra−24. The terrain feature continuation executes its own height adjustments before these calls; they are observed consumer results, not inferred from the terrain fan. All20 admitted calls use this rule across the four worlds. The normal viewer reads remembered improvement bits from its tile snapshot unless its revelation status selects live bits; this fixture explicitly sets viewer3's snapshot to the live feature bits. A river-bit control produces no call to these sprite helpers; its terrain rendering is outside this sprite supplement.

An ordinary nonselected Unit sharing a valid Base tile was suppressed by the actual dispatch in this fixture. The Scout-only cases remove the Base marker and execute the same native map loop again. Do not use these receipts as proof that every occupied Base always draws a separate Scout model. Selected-unit exceptions and blink/timing retain their existing contracts.

Scope: original object iteration, native Base/Unit lookup, renderer dispatch, overlay rectangles and current-Windows-GDI text coordinates execute. Terrain paint, model pixels and Base sprite pixels are sinks in the iteration probe. Model surface setup has an independent original-model replay. Native sprite metadata/destination receipts, when present in the cases, stop before clipping/pixel copy. This is a coordinate contract, not a whole-game screenshot or historical font parity claim.
