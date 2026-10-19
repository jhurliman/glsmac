# Private audit: minimap producer, camera, loaded-slot color

Pinned terran.exe SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Public `behavior/minimap-world-detail-camera.md`, named-field cases JSON and `minimap_navigation_reference.py`.

## Producer execution

`probe_minimap_camera.py` composes only the writer-free console/terrain fixture prefixes. Full original `46f110(1)` constructs separate supplied detail/active view objects. Original `4d9a90` transfers the native minimap rectangle into91f440 and marks its backing bit8; this fixes an initial exploratory detail initialization that had used an uninitialized rectangle and is not accepted.

Full `47d630(detail,1,0)` takes the detail-pointer special case (`7f0b58`) and creates the child window at native console rectangle780,88,1019,207, ending zoom−7/flags80000082. Actual `46f2a0` reads supplied focus index0/coordinates8,8, producing that center. Full `4da9d0(overview,1)` resets +239bc/+239c0/+239cc/+239d0, sets internal view +1dd78=1, initializes it through47d630, creates the outer window and marks geometry dirty8. Global overview is8f3c68, internal view8f5194 (+152c).

Native window creation5edec0 and resize5ee360 are generic presentation services: record native rectangle/size requests, publish width and signed negative height into the actual native Window backing, and return. No view flags/centers/spans/map-selection results are supplied. Show/hide are observed sinks. Full overview producer4d9ef0 executes all branch/arithmetic/cache-selection code, actual bitmap allocations and native geometry setup. Base draw476da0, overlay476390 and bitmap-copy5f8d00/scale5f2410 are observation sinks. Source color grid is not rerun. Source renderer flags80000000 then80000080, zoom−14 and center40,40 are captured directly; source320×152/target239×119 and later unscaled400×200 padding40,24 are native requests. `46f3b0` is executed separately after the producer to obtain actual first/span geometry, rather than pretending the sunk draw filled it.

4d9ef0's initial geometry branch computes base unit1 at ordinary zoom−14 and sets a14=320,a18=152 for80×80. If its window fits both source dimensions, +239c4/+239c8 store centered integer offsets. Otherwise both are0 and the native9-arg bitmap-scale request fills the whole target. The normalized coordinate helpers use world dimensions and target/image dimensions directly; the internal base-geometry first coordinate1,1 is distinct.

## Navigation execution

Complete4da420 maps local pixel→doubled-X tile, replacing Y parity from X, returns0 success/1 failure. Complete4da500 maps tile→local pixel, wraps X once only for round world, clamps Y, and applies native integer scale/padding. Both execute for29 inputs across stretched and letterboxed contexts. Output buffers are poisoned; public failed cases expose semantic absence instead of stale/partially written output values.

Full4d9d70 executes three cases. Native KeyState import68b35c is supplied0 or8000 for Shift key16. Ordinary valid click reaches actual477a40(active,39,39,1); Shift valid click reaches actual477a40(detail,activeCenterX,activeCenterY,1), then selects1007, shows detail and hides overview. The active center fixture is22,30. Invalid pixel239,119 reaches no action. Full477a40 executes parity/wrap/center stores; display cleanup4d42c0, overview invalidation4d9ed0 and draw requests are sinks. Active-view window backing1024×511,zoom0,flags10ee3 is explicitly supplied for this recenter fixture, not produced by a full active-window startup. Command-handler original stack cleanup is asserted.

Source wrappers4d9e80 supplies(x,y,0,0);4d9ea0 supplies(x,y,1,0), entering the context-menu branch. The latter menu is source-mapped only, not executed here. The handler's fourth-argument producer is not given a new semantic mouse-button label. Shift behavior is independently executed through its original GetKeyState check.

Full46f3b0 and4da810 execute six enabled-outline cases.4da810 reads the view pointed to by7f0b58: first1dda4/8 and span sums1ddcc+1ddd4,1ddd0+1ddd8. It calls actual4da500 twice; observes5fc003 frame or5fa880/5fa9f0 line submissions. IndexFBh=251. Source handles seam x1>x2 by six adjusted segments. Flat negative endpoint failure suppresses the outline; Y projection clamps at poles. Outline enable at overview+239d0 is supplied1; blink producer/cadence is excluded.

The independent named model matches29 helper returns/outputs and all six view first/spans/outline packets. It uses signed truncation, not Python floor division for negatives. Tiny-map centering and scale override are explicitly outside its domain.

## Slot assignment and map colors

`probe_minimap_slot_assignment.py` executes actual default palette slot-table producer45f6b9..45f72f, then full59de20. The emulated FILE-open service supplies original installed Alpha bytes with only FACTIONS rows1/3 swapped; both Alpha opens receive the same virtual bytes. Host installed files are untouched. Full59de20 reads BONUSNAMES/FACTIONS, copies rows to sequential definition slots beginning95aa64, stride59c, then calls full59d1d0(record,0) and45eb70(slot) for all seven.

Each complete faction text parser reads its actual installed faction file. Full45eb70 selects the filename from definition+4, loads the actual matching PCX through native5f0be0/PCX decode with palette argument0, and runs its original sprite argument construction. Actor/cache producer4297e0 and actual sprite crop/storage5fc7d0 are presentation sinks; neither source body writes the primary map-color table. These excluded graphics paths are not presented as complete actor/faction graphics initialization.

Memory-write hooks span6aeeb4..6aef33 (four slot-color tables) and959aa8..959ea7 (active palette) during the list/parser/faction-image run. No writes occur; snapshots match. Parsed names/filenames establish University in native slot1 and Gaians in native slot3. Full46fd00 political own-claim cases consume slots1/3/5 and paint241/255/65, respectively. A fourth case mismatches renderer viewer3 with display faction1, both explored, own claim1; native political color remains241. The report's phrase “three full renderer identity controls” counts the three identity cases; the fourth is the observer discriminator.

The same table entry and active palette retain their values through actual custom-order parsing and PCX loading. This establishes loaded-slot authority in this custom discriminator, not a filename→canonical-color rule. Existing setup transfer confirms that human choice preserves loaded definition slot. No arbitrary game-time reassignment, custom palette replacement, network player reorder algorithm or discarded import provenance is invented.

Political admission at470944..4709b6 reads global9430d4 display/focus faction, while earlier470739..47077b admission uses renderer arg+14h. Native4f97a0 then filters claim+7 through that display faction's recognition entries. This distinction was exposed by the initial identity test leaving display3 while supplying viewer1; that failed diagnostic is superseded by the explicit accepted observer contexts.

## Immutable supplement

`tmp/reference/minimap-world-detail-camera-evidence-20261019/manifest.json`

SHA256 `901f4ffe0adb056144a56b98e5b6d7c7e0977dd2d1ea37a24ef9516be21d5b4a`.

Bundle contains new probes, accepted logs/raw receipts, original source listings, immediate harness dependencies, checker/finalizer, public cases and named reference. Nested earlier dependencies remain in the reference tree and prior frozen bundles; this is an audit snapshot, not a standalone redistributable emulator. The earlier minimap/button/color bundle is unchanged. Full pixels, four-button reference, UI political selector and save/load lifecycle remain unresolved. No production source or owned asset was edited, and no broad goal was resumed.
