# Ordinary land texture selector, composition and palette

This finite handoff connects the existing height/span/shade contracts to actual land art and the active color table. [Executable selector](land_texture_selector_reference.py), [39 native contexts and ordered calls](ordinary-land-texture-palette-cases.json). Original atlas pixels stay private. The new probe executes selectors/composition while observing raster calls; existing raster/height/span results are reused.

## Producing fields and base art

For valid ordinary land, moisture is `(tileByte0>>3)&3`, altitude band is `tileByte0>>5`, and rockiness is `tileByte5>>6`. Accepted moisture0/1/2 means dry/moist/rainy; rockiness0/1/2 means flat/rolling/rocky. Invalid code3, landmarks and special render modes are excluded. Tile+8 holds live items; ordinary viewer3 uses its remembered item word at tile+24 unless revelation status selects live items. Fixtures initialize both consistently and are known to viewer3.

Dry land selects TEXTURE.PCX(1,58),56×56. Moist and rainy select one of sixteen56×56 crops, respectively grids x=1/58/115/172,y=115/172/229/286 and y=343/400/457/514. The exact selected index and UV rotation come from an8-neighbor connectivity mask and the original256-entry lookup (retained as numeric rule data in cases).

For mask bit i, use native neighbor_offsets[(i−1)&7]. Invalid neighbors contribute no bit; round-map X wraps, Y does not. A valid sea neighbor sets the bit. A land neighbor sets it when neighbor moisture>=center moisture. Lookup returns crop index and quarter-turn UV selector. Moist/rainy rotate the four canonical UV triples by that selector; dry uses the canonical triples unchanged. The reference verifies actual crops and UVs against all39 contexts, including cardinal/diagonal discriminators, sea contact, round seam, flat edge and north pole.

Rockiness adds a keyed shaded detail triangle **immediately after each base triangle**. The native flat fan is four calls; rolling/rocky fans alternate base/rock for eight calls. Do not batch all four rock layers after all bases. Rockiness1 selects(1,1) or(115,1); rockiness2 selects(58,1) or(172,1), all56×56. The choice is the local variation described below. Every literal call in the cases retains crop, vertices, UV, signed light and flags.

| Uniform profile | First native selections, then repeated per fan | Calls |
|---|---|---:|
|m0_r0|[1, 58], then [1, 58] …|4|
|m0_r1|[1, 58], then [115, 1] …|8|
|m0_r2|[1, 58], then [172, 1] …|8|
|m1_r0|[115, 286], then [115, 286] …|4|
|m1_r1|[115, 286], then [115, 1] …|8|
|m1_r2|[115, 286], then [172, 1] …|8|
|m2_r0|[115, 514], then [115, 514] …|4|
|m2_r1|[115, 514], then [115, 1] …|8|
|m2_r2|[115, 514], then [172, 1] …|8|

## Variation seed is an address dependency

The original renderer initializes a local RNG, seeds it with the native tile-record address, then requests range[0,2). Its update is `(seed*1664525+1013904223) mod2^32`; output is `((state&65535)*2)>>16`. Thus only the address's low16 bits affect this choice. This is not a newly consumed gameplay/presentation RNG stream or a coordinate-only selector.

Moving the emulated map allocation by65536 preserves the choice. Moving it by4 flips the measured choice1→0; actual Farm and rolling-rock calls then change crops. The portable selector therefore takes explicit `variation_seed_low16`; a compatibility caller can derive it from `(nativeMapBaseLow16 +44*(rowWidth*y+x//2))&65535`. **The original live allocator's base-low16 stability is not established.** Do not silently replace it with a new hash of coordinates and claim exact art variation.

## Farm, Forest and Xenofungus

With render flag0x1 set, Farm item0x8000 replaces the base crop with Farm bank index `3*variation+moisture`: x=775/832/889, y=219/276 for the measured variation0/1 choices. The bank also contains a third row at333; its production is not established by these controls. Farm uses canonical UVs without the moisture-pattern rotation. Existing standalone TER1 Farm sprites are a separate flag0x20 path and are excluded here.

Forest item0x200000 leaves the base fan, then adds one keyed shaded quad over its W/N/E/S geometry. Four diagonal neighbor offsets, in the cases, form a direct4-bit Forest index; crop grid x=526/583/640/697,y=6/63/120/177. An isolated Forest selects(526,6); a Forest neighbor NW selects(526,120). A W neighbor at(−2,0) does not set a diagonal connection. These calls retain four corner lights.

Xenofungus item0x20 leaves the base fan, then adds a keyed **unlit** quad. Its8-neighbor Fungus mask uses the same pattern lookup as moisture, with connection requiring Fungus plus neighbor band>=2. The selected land grid starts(280,516),56×56; the cases retain its exact non-square UV corner offsets. Isolated Fungus selects(280,516); NW-connected Fungus selects(337,516). Farm+Forest and Farm+Fungus controls confirm Farm base replacement followed by the appropriate quad. Other combinations, full shore/river/fungus transitions, sea fungus and feature sprites are not generalized here.

For known tiles the opaque and rock wrappers carry flags−1, delegating to full shader variants. Base triangles write sampled+shade indices. Rock/Forest compare the loaded index to key9 **before** adding signed shade; key9 preserves the underlying destination. Fungus compares key9 and otherwise writes the loaded index without vertex shade. These shader distinctions follow the actual called variants and their inner stores; this probe supplies no new pixel-parity claim for keyed quad/rock coverage.

## Active palette and final RGB

Use common **PALETTE.PCX**, not TEXTURE.PCX's embedded palette, as the active color authority. Source palette entries0..235 populate game entries10..245. Texture pixel indices also rebase by+10 before rasterization, so source48 becomes loaded58. The opaque shader then adds signed shade with byte wrap; apply the active palette **after** that index operation. Keyed shaders perform their key test on the loaded index first.

The native Gamma Correction100 profile uses per-channel `trunc(235.99*clamp(pow(c/235,1/gamma),0,1))`, gamma1, giving min(c,235) for the checked8-bit common entries. All236 common entries match this rule; loading TEXTURE.PCX leaves the active palette unchanged. The bitmap receives that table through the native palette upload, with RGB converted to BGR0 for the indexed DIB. All256 uploaded RGB values match the active table. System/reserved entries outside10..245 are retained from initialization and excluded from the gamma loop; the public common-palette helper requires a supplied active table for those entries rather than inventing colors.

| Final game index | Native Gamma100 RGB |
|---|---|
|10|[172, 129, 81]|
|22|[80, 65, 51]|
|53|[122, 65, 51]|
|54|[113, 61, 49]|
|55|[104, 56, 47]|
|56|[96, 51, 45]|
|58|[79, 41, 41]|
|60|[62, 31, 37]|
|65|[0, 0, 0]|
|148|[39, 31, 12]|
|245|[193, 173, 26]|

The cases include separate hashes for the common corrected RGB range, native palette state and actual DIB color table. Entire palettes and owned pixels remain private. Different gamma, system color management, dynamic palette effects and whole-window presentation are not newly closed by this profile.

## Reproduction and limits

Run `python land_texture_selector_reference.py` to verify all39 ordered selection contexts; optionally pass `--palette-pcx PATH_TO_OWNED_PALETTE.PCX` for the finite color checks. Rule tables/crops/UVs are numeric data, with no asset pixel bytes. Native texture atlases were actually loaded/extracted in the private probe;160 bindings are saved privately.

Measured renderer flag1, known viewer3, ordinary land, world80×80, zoom0/default relief. Camera and boundary controls serve selector/UV checks, not new camera or raster guarantees. General original allocation-base stability, other zoom/flag presets, landmarks, fog/political modes, sea/shore/river composition, invalid retained codes and nondefault gamma remain unsupported. This handoff establishes the tested producers and ordered outputs without claiming whole-scene fidelity. Broad RE remains paused.

## Closed quad UV and flag producer

The executable selector now returns and strictly checks UVs and flags for **every** call, including all Forest/Fungus quads. Forest UV is fixed[0,0;55,0;55,55;0,55], with flagsNone because that full helper has no wrapper flag argument. Fungus flags areFFFFFFFF in these known-land contexts.

Fungus obtains `patternTurn` from the same original8-neighbor connectivity lookup used for its crop. For each quad corner j=0..3, native code takes **vertex1** of canonical fan UV triangle `(patternTurn+j)&3`. This gives a rotated non-square sequence; preserve55,1 rather than normalizing it to55,0. The executable implements that producer, rather than selecting a fixed square or matching context names.

| Quarter-turn | Actual native control | Quad UV in submitted vertex order | Flags |
|---|---|---|---|
|0|fungus|[0, 55, 0, 0, 55, 1, 55, 55]|0xffffffff|
|1|fungus_connected_NW|[0, 0, 55, 1, 55, 55, 0, 55]|0xffffffff|
|2|fungus_connected_SW|[55, 1, 55, 55, 0, 55, 0, 0]|0xffffffff|
|3|fungus_connected_SE|[55, 55, 0, 55, 0, 0, 55, 1]|0xffffffff|

The isolated and NW controls reuse earlier native receipts. Only SW and SE were newly executed to close turns2/3. All original37 contexts/call records remain unchanged. The revised39-context/193-call check compares crop, UV and flags unconditionally on every call;9of those calls are quads. This adds selector observations, not new quad raster/pixel or different-zoom parity claims.
