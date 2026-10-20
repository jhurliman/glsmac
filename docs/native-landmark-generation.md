# Native generated landmark placement

`PlaceLandmark` implements all thirteen original placement operations over the
owned raw plane: Jungle, Crater, Mount, Sunny Mesa, Ridge, Uranium, Ruins,
Manifold Nexus, Boreholes, Sargasso, Great Dunes, Freshwater and Geothermal.
The name table is a caller-owned encoded/localized input; stock English names
are available separately from the placement rules.

The actual seed-1 test starts from zeroed allocation and independently executes
the prior generation and initial-climate stages. It matches the original
140,800-byte plane after every landmark return, all region bytes, dirty flags,
ordered CRT draws, label positions and the twelve 40-byte name records. Great
Dunes consumes its thousandth coordinate pair and fails placement; there are
thirteen calls but twelve labels.

After the last landmark, before contour normalization, the plane SHA256 is
`f5e2741ce6780c778d3247c9e4085a096c4e284347745f8fea94a76bd0a8b7cf`,
CRT is `2381366168` and shared state is `193599486`. Placement consumes 2,957
CRT draws. The 480 name-record bytes hash to
`238faa37f0237b0889140f3cb12027a0817eb120c46ae8faed4dea4cbe6b5161`.
These are intermediate producer outputs, not a completed playable world.

## Rules and shared implementations

Placement uses ordered footprints, not distance approximations. The first 225
offsets reuse the proven shaping table; the outer 64 and thirteen non-sorted
ridge indices come from immutable executable data. The existing native shaping
kernel now exposes the direct shape operation at the prelandmark boundary.
Continent admission remains separate from landmark shaping.

Jungle and Great Dunes recompute rainfall even when an explicit center is
supplied or placement fails. The existing rainfall algorithm is reused through
an owned semantic view, publishing only moisture, auxiliary and landmark fields.
Raw pre-preparation temperature/moisture codes occupy three/two bits; the shared
view now admits those storage ranges. Selected climate settings still require
0..2. Ordinary prepared worlds' results remain unchanged.

Borehole orientation takes an explicit external clock value, not a game RNG
draw. Freshwater scans stored sea-region areas, resolves ties toward later IDs,
then scans rows descending while retaining its first matching X across rows.
An all-matching plane can receive markers without a label. Names preserve
out-of-bounds native label offsets; serialization bounds text to the native
32-byte slot rather than reproducing the original overlong-string corruption.

## Controls and validation

Seventy-three original controls cover explicit and random placement, wrapped
and clipped edges, failed searches, all 32 borehole orientations, suppressed
Mount labels, membership/detail distinctions and corrected freshwater
tie/all-matched cases. They verify complete tile/region output, labels, dirty
flags and **14,196 ordered CRT draws**. Ordered tile stores are also checked for
the eleven operations without semantic rainfall; Jungle/Great Dunes compare
complete output instead. No globally interleaved instruction trace or rainfall
store-order claim is made.

The corrected freshwater controls come from the v2 supplement. Its manifest
SHA256 is `14bf175ce2548817e73cf887cc5ecdcbfd8d4cf0de8d5f7d07b023cc9796bdc4`;
all 106 files were verified before import. The inherited original tail freeze
has manifest SHA256
`e617d515b91a3bec9f83c2ea0e7b469b94c386d099f84eb3273d0435f53265c3`.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Numeric observations are fixtures only; production never loads captured helper
outputs. Purchased art and executables stay outside Git.

The cached incremental build, fourteen affected native groups, correctness
analysis of six production TUs and focused six-TU ASan/UBSan checks pass. The
sanitizer scope covers landmarks, shared shaping/rainfall, RNG and their test
consumers. No local platform matrix repeated.

Next are profile preparation and the final climate/routing controller, then
faction startup and the [live generated-world integration](native-generated-world-integration.md).
Current gameplay rendering remains unchanged.
