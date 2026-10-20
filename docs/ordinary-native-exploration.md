# Ordinary main-map exploration presentation

Normal SMAC gameplay on the retained, wrapped 80x80 Planet profile now uses the
owned native view renderer without `--native-terrain-inspector`. It passes the
authenticated observer's saved exploration and survey rule into ordinary view
preparation. Known terrain, remembered improvements, survey presentation and
unknown black/grid outlines use their existing original-backed drawing paths.
The explicit inspector continues to select its separate revealed canvas.

Units and Bases keep their independent existing disclosure policies. The native
view's selected object window additionally crops their render roots; it does
not replace discovery, head/representative, ownership or primary-name rules with
one terrain-known Boolean. Ordinary native views now share the inspector's
discrete zoom, canonical/wrapped actor anchors and committed-frame picking.
Zoom dispatch runs after UI handling, before other unconsumed gameplay keys.

The legacy terrain mesh and replaced props are suppressed from initialization,
including newly delivered prop instances. They cannot appear beneath unknown
areas while an asynchronous snapshot is pending. Missing authority or an
unsupported native profile presents opaque black, hides object roots and owns a
valid miss instead of falling through to legacy GPU picking. A stable blank
canvas is not uploaded and republished every frame. World retirement clears the
canvas, view history, mask, pending picks and presentation context together.
Actor visibility delivery also requests a fresh terrain snapshot, so new sight
reaches the main map independently of texture updates.

## Live validation

Two hidden Linux SDL/OpenGL runs used the cached native executable and no
inspector flag. The first loaded Morgan 2105 from campaign SHA256
`aed85217e7684412e0de762d4ccf23df5892f20fc0355a8b2fa15ea55580ae92`,
advanced through 2106/2107, answered the research-priorities prompt, moved a Scout
north from (34,68) to (34,66), and observed the newly revealed terrain/preview.
It saved through the UI to campaign SHA256
`dc4b29d9860f572668418515b336a4af3ab93de6319ce09af06040f41b659f55`,
reset to the title, reloaded that save, verified the retained explored patch at
minimum zoom and opened Morgan Industries through a map click. The second run
checked final zoom dispatch after UI handling and returned to default zoom.
Both quit normally with exit0 and no tracked OpenGL leaks; audio and allocation
debugging were disabled. These are gameplay integration runs, not performance,
audio or original-executable framebuffer comparisons.

The following are unaltered 1024x768 framebuffer captures. The movement and
reload captures precede only the final keyboard-handler routing change; the
final default capture comes from the second run.

![Ordinary observer view after load](https://raw.githubusercontent.com/jhurliman/glsmac/79f58d683579c354a7fbe90cf073d4e2708cdb73/ordinary-native-exploration/final-loaded.png)

![Scout movement reveals northern terrain](https://raw.githubusercontent.com/jhurliman/glsmac/79f58d683579c354a7fbe90cf073d4e2708cdb73/ordinary-native-exploration/north.png)

![Reloaded exploration retained at minimum zoom](https://raw.githubusercontent.com/jhurliman/glsmac/79f58d683579c354a7fbe90cf073d4e2708cdb73/ordinary-native-exploration/reloaded-overview.png)

Cached incremental builds pass. Final selected native validation passes65
groups and a focused13-group actual Unit/Base/preview pass. The latter explicitly
checks off-view body, name and overlay removal/restoration without replacing
ordinary disclosure policy. Earlier validation also
includes the new saturated-corner regression. Four changed production consumers
pass correctness analysis; Game was rechecked after final input routing.
A focused11-TU ASan/UBSan build passes55 native groups and the13 actual
actor/preview groups, with other objects uninstrumented and leak detection
disabled. The final keyboard route is separately checked by native tests,
correctness analysis and the second live run. No local platform matrix was
repeated.

## Remaining requirements

This connection does not generalize the retained terrain authority to generated
maps, arbitrary sizes, flat maps or SMAX. Unsupported native sessions deliberately
remain unavailable rather than exposing hidden legacy terrain. The existing
non-native map path has not gained faithful unknown rendering from this change.
Generalizing those authorities remains required for1.0.

View/profile persistence is still missing. The reload run restored exploration,
Units, Base and turn, but initially centered (40,40), an unexplored area, because
no active Unit supplied focus. Minimum zoom exposed the saved known patch. Do
not describe this as saved camera restoration. Display-preference ownership,
foreign-object interactive scenarios, arbitrary pan histories and additional
map/platform integration still need their acceptance passes. Full actor/font
matching, minimap presentation and initial console-card population also remain
open. The observed initial card population0 versus map population1 is not hidden
by these captures; upkeep refreshed that card to1.
