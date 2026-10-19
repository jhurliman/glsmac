# Native source map: landmark-field geometry admission

Pinned terran.exe SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Contract: `behavior/terrain-landmark-field-admission.md`.

47ca10 validates coordinates and returns0 for band<3 at47ca64..47ca7a,
before the +0xc load47cac7. On cache miss the signed detail is SAR24 at47cad1.
Height profile tests47cad4,47caef,47cb09,47cb23,47cb3d are respectively
2,1,200,100,800; lookup6a8654[detail*5+vertex], then profiles
7f0a8c,8134e8,813474,7f0990,813400. No signed-detail guard intervenes.

47d0f0 sea short circuit47d185..47d19f publishes128 and returns0.
47d330 adds profile shade through47d363..47d3bf: tests1,2,200 in that
order; each SAR24 is followed by JS to47d3c2. Tables6a89f4,6a8b30,813668.
Ordinary face/corner averages follow. Direct shade query47d330 is distinct
from its sea renderer admission:46fd00 at4709e0 branches on band<3 to
470a4d, publishing supplied water scalar; land calls47d330 at470a44.

46fd00 at46fdb7 tests400000 only for the special receiver8f5194 and
when renderer bit80000000 is clear: this is a dirty-only admission path,
not height/shade deformation. Existing map-import evidence identifies the bit
as dirty bookkeeping and records it on all3200 installed Planet tiles.
Existing ecology-class-setter evidence records native OR400000 writes after
terrain/rockiness changes.

Separate material paths include neighboring Jungle4706a9..4706cf and
47178d..4717b6 (mask80000004, equals4, neighbor land check), Uranium
4716ae..4716d7 (mask80000008 equals8), and Geothermal4716d9 onward
(mask80000400 equals400). This supplement does not close their art.
Freshwater membership is80, not400. Shared high-byte writer/lifetime evidence
is already in re-smac-shared-landmark-detail-lifetime.md; no new bit meaning
is inferred from sample numeric values alone.

New bounded control source: tmp/reference/probe_landmark_metadata_geometry.py.
It reuses the writer-free setup of probe_terrain_geometry.py, calls complete
native47ca10/47d330/46f930 with fresh cache, and compares metadata-only words
against an unmarked baseline across bands2/3/4. It does not execute raster
stores or claim unsupported material parity. Results are kept separately from
all earlier frozen archives.
