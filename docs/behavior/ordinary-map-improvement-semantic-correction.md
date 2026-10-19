# Ordinary map improvement semantic correction

The first object/loaded-sprite handoffs mislabeled all six tested improvement controls. This revision supersedes those semantic names. Their raw-coordinate evidence remains valid, and all24 prior loaded rectangles match the corrected identities exactly. Base/Scout anchor and height findings are unaffected. Original frozen bundles are unchanged.

| Raw native item mask | Correct identity | Wrong v1 name | Native TER1 source crop origin |
|---|---|---|---|
|0x10|Mine|Farm|(607,64)|
|0x40|Solar Collector|Mine|(607,127)|
|0x400000|Condenser|Solar|(506,253)|
|0x80|River|Forest|terrain path; no isolated sprite rectangle claimed|
|0x800|Bunker|Sensor|(506,316)|
|0x40000|Airbase|Bunker|(607,316)|
|0x80000000|Sensor Array|not previously tested|(708,316)|
|0x8000|Farm|not previously tested|(923,453), isolated land variant0|
|0x200000|Forest|not previously tested|TEXTURE.PCX terrain quad bank|

The original action/ecology mask authority already established Mine/Solar/Farm/Condenser/Forest and the River producer; native item render tests plus native atlas extraction distinguish Bunker/Airbase/Sensor. Existing port Consts.h agrees with the listed crops; no port definitions were changed to fit this correction.

The corrected native probe executes48 contexts: the four terrain worlds, nine real raw-mask controls at render flags0x20, plus Farm/Forest/River controls with flag0x1 additionally enabled (0x21). Seven real improvements each reach a sprite helper at the projector anchor. Farm0x8000 also selects a different terrain texture when flag0x1 is enabled. Forest0x200000 at0x21 reaches a four-vertex terrain-overlay draw using the actual W/N/E/S heights. It does not use a freestanding sprite anchor or fan-center height. Forest at0x20 produces no such overlay; this is a display-flag boundary, not absence of Forest art. The River0x80 controls are retained without claiming an isolated river sprite.

New loaded native Farm and Sensor rectangles are measured across all four worlds. Combined with the corrected prior identities,32 rectangles pass96 field assertions. Forest contexts close its native submitted quad geometry only: texture pixels, connected-neighbor variants and final composition remain outside this correction. Neither absence of a sprite call nor an uninitialized texture receiver is treated as artwork fidelity.

Use the revision2 ordinary-map-object-anchors-cases.json and ordinary-map-loaded-sprite-rectangles-cases.json. Their semantic_revision field records this correction; every sampled feature now includes its raw mask. The original48 Base/Scout/old-raw-coordinate checks remain historical evidence; they are not48 independent semantic cases. Corrected feature execution has48 contexts.

Reproduction uses corrected probe_improvement_anchors.py and probe_object_sprite_bounds.py, followed by reissue_improvement_semantics.py. Legacy finalizers are retired to prevent regenerating the wrong labels. This correction makes no production or asset edits; the bounded terrain-raster handoff remains unchanged.

Delivery split: the canonical object/rectangle files retain their original48-check/24-rectangle scope with corrected labels. New Farm/Sensor rectangles and Farm/Forest/River display-flag contexts are isolated in [the expanded supplement](ordinary-map-improvement-expanded-contexts.md) and its adjacent cases. The earlier combined revision2 evidence remains frozen; this split changes packaging, not native findings.
