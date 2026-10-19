# Ordinary Road and Tube overlays

**61 native producer contexts / 335 ordered commands; 65 composed indexed
frames / 10,364 spans / 360,796 pixel attempts / 169,846 stores** match the
portable [reference](road_tube_reference.py) and
[cases](ordinary-road-tube-cases.json). Native execution stops immediately
after the Road/Tube branch, before later objects; it is not represented as a
full tile-renderer return. Profile: known viewer3, zoom0/default relief,
flags`0x21`, plus one flags`0x421` River control. Owned pixels stay external.

## Admission and inputs

Renderer flag **`0x20`** enables the branch. The center must be land and have
selected **Road bit`0x4`** or an admitted Base. Tube bit`0x8` **alone does not
admit a non-Base tile**. Do not repair that controlled raw-state distinction.

Selected Road/Tube bits in this profile come from the viewer's remembered
item words, for both center and neighbors. They differ from the live River
input. Live-only/remembered-only controls establish that source; general fog
and knowledge-update timing are not added here.

The actual Base query requires the live Base marker and a matching Base record
at those coordinates. For viewer3, admit it if owner3 or its Base-visibility
byte contains bit`1<<3`. Tile visibility and territory ownership do not replace
this Base-specific test. Missing/mismatched Base records are not fabricated.

## Selection and isolated patches

Visit NE,E,SE,S,SW,W,NW,N in order, with doubled-coordinate offsets
`[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]`.
Wrap X once on round maps. Skip invalid flat X and all invalid Y.
The neighbor must be **land** and have selected Road`0x4` or an admitted Base.
Sea does not connect even with Road bits. There is no extra region, territory,
diplomacy, elevation-equality or flow-direction test in this branch.

Center Tube mode means selected`0x8` **or** admitted Base. For each qualifying
neighbor, draw a Tube link if the center is in Tube mode and that neighbor has
`0x8` or an admitted Base; otherwise draw a Road link. Each is a separate
whole-tile quad. For neighbor index`d`, select family index
**`1+((d+2)&7)`**. There is no combined mask texture or UV rotation.

After links, an admitted **center Base suppresses all isolated patches**.
For a non-Base center, append isolated Tube if in Tube mode with no Tube link;
otherwise append isolated Road only if no link at all was drawn.

Consequently, a Tube center connected only to Roads gets Road links and then
an isolated Tube patch. A mixed junction with any Tube link gets no isolated
Tube patch. A Base without connections gets no Road/Tube quad in this branch;
later Base artwork is separate.

## Art, geometry and pixels

Each family has nine56×56 crops from external `TEXTURE.PCX`: index0 isolated,
indices1..8 directional. For index`i`,
`x=775+57*(i%3)`, `roadY=395+57*(i//3)`, `tubeY=566+57*(i//3)`.

Use the existing deformed **W,N,E,S outline**, its four shades, UV
`[0,0,55,0,55,55,0,55]`, and scalar bounds`[-4,+4]`. There is no height lift,
new mesh or screen-space sprite anchor. A raised-center control checks this.

The source orders Road/Tube after territory, coast and River. Pixel fixtures
include coast, River, Forest and Fungus combinations. Simultaneous territory
plus Road is source-ordered but not an extra composed frame in this handoff.
Later object/improvement branches stay outside the finite prefix.

Use the existing keyed-shaded raster: loaded key9 preserves destination
**before** lighting; other pixels write`(loadedIndex+signedShade)&255`.
Shaded half-texel UV initialization and established clipping/interpolation
apply. No CLUT, RGB alpha blend or new palette is constructed. Repeated links
can store to the same pixel several times; preserve native draw order.

## Validation and limits

All eight directions cover Road/Tube center-neighbor combinations. Other
controls cover isolated/mixed/all-neighbor cases, Tube-only/flag/sea rejection,
selected/live disagreements, Base visibility and implicit connections, west
wrap/flat, both poles, raised geometry, feature ordering and four clipped
replays. Native atlas, geometry and Base helpers execute; polygon sinks capture
plans. Separate native raster replays and an independent reference match every
span, attempt/store hash and complete indexed frame.

Run`python docs/behavior/road_tube_reference.py` for producer checks. Add
`--texture-pcx` and`--rainfall-pcx` paths to installed originals for the65
indexed-frame checks. No original art is embedded in public cases. Other
viewers/fog states, zooms, landmark geometry, subsequent objects, full-scene
scheduling and playable transport rules remain separate.
