# Native map Unit overlay placement

The original populated viewport places the complete23x30 flag canvas at the Unit dispatcher helper, with its2x21 health backing at helper+(6,6). See [original populated viewport](viewport-populated.md). At zoom0 the visible Scout helper is624,288 and health backing is[630,294,632,315).

The former actual Camera/actor path projected the flag at24x19.2 pixels and shifted its upper-left by(11.478271,19.199982) relative to the helper. Source texture dimensions alone did not establish correct map geometry.

Supported conventional CVR definitions now share their existing native-body admission predicate with the overlays: native map projection, standard1x1 descriptor/128,128 center, zoom0 and default or unity scale. Known health is required. Independent map actors use source-sized rectangles in the96x48 native tile projection and top source pivots; the flag and health follow the helper and its(6,6) offset through both Show and coordinate updates. Generated texture owners remain retained by their map actors. Console actor geometry and custom descriptor scaling remain independent.

For admitted original faction palette slots, the map flag uses the same indexed UNITS.PCX bank/palette/glyph producer as the console. The two health columns are transparent for the separate health actor. Variant1 applies only to an owned Unit without remaining movement. Unknown original palette identity retains fallback art with corrected map geometry; that fallback is not original palette parity.

The actual frontend Unit/Scene/Camera fixture checks flag and health rectangles at heights0,25,6,3 for Scout, initialized Scout and Rover. It verifies the admitted map bank/glyph pixels against the separately retained card producer outside the health opening, while retaining existing independent console/custom descriptor and teardown checks. This is not complete populated-frame, historical font, shadow or arbitrary-camera pixel parity.

Validation: three focused fixture groups pass in the cached native build. Eight affected production translation units pass correctness analysis, with the final Unit.cpp change rechecked. The same three groups pass with eight affected translation units instrumented by ASan/UBSan; other linked objects are normal builds and leak detection is disabled.
