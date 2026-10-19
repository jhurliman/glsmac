> Revision2 semantic correction: see behavior/ordinary-map-improvement-semantic-correction.md. Frozen v1 names are superseded; raw evidence remains unchanged.

# Loaded sprite rectangle execution

Pinned terran.exe SHA2560a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01. Native5f0be0 loads TER1.PCX; original45e1e4..45e5c4 extracts bunker/airbase/mine/solar/condenser banks using full5fc7d0. Full45eb70(3) loads/extracts UNIV.pcx with4297e0 actor/cache sink. Native45ee50 constructs77e4a0 artwork pointer table. Base+5 is explicitly3 in this supplement;5719b0 executes to its5fe680 call, whose receiver/args are captured. Recorded ordinary-map anchors are replayed for every fixture, and native unscaled5fe680 is stopped at5fe7b9 after constructing its[ebp−1c]rectangle. Improvement5fd9e0 is stopped at5fdb8c after constructing[ebp−20]. Rectangle offsets/width/height match loaded sprite metadata+20/+24,+10/+14 respectively.

Repeated5f7380 substitutions use an identity sink only if the donor byte does not exist anywhere in the actual bitmap. Source clip is explicitly reset to its original complete bitmap bounds. Failure history includes native palette scan instruction caps at5f74c2 and the missing45ee50 binding (retained log). No initial failed attempt is claimed as success. This independent supplement leaves the earlier object-coordinate frozen bundle unchanged.

Reproduce probe_object_sprite_bounds.py then finalize_object_sprite_bounds.py. Immutable bundle: tmp/reference/ordinary-map-loaded-sprite-rectangles-evidence-20261019.
