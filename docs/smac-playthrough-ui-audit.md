# SMAC honey playthrough: presentation acceptance

The user's 2026-10-19 native Mac playthrough is direct evidence of unfinished
presentation. A believable map or a successful launch is not screen fidelity.
The original screenshots supplied in this thread define the target; their
remote-desktop/Retina scaling must be normalized before image comparisons.

| Area | Current evidence and cause | Required acceptance |
| --- | --- | --- |
| Logical resolution and console | The Mac launch used1280x960. The viewport preserves4:3 but exposes the full physical dimensions; the console remains257px,26.8% of960 instead of33.5% of768. | Render the complete scene at1024x768 logical pixels, with511px map and257px console. Present with nearest integer scaling and bars; translate input through physical/drawable/Retina coordinates. Match original default zoom before comparing visible tile counts. |
| SMAC setup buttons | Only the title used the original blue compositor; later setup pages used hardcoded green classes. | Every SMAC setup branch uses original blue artwork and palette. Preserve SMAX selection and Back/Cancel behavior. Menu headings must not shift action indices. |
| Faction selection | `select_faction.gls.js` presents a generic list and calls start_game directly. Original`Script.txt` sections`#FACTION`, `#NAME`, `#NAME2`, `#CUSTOMNAMES` describe the missing flow. | Match faction portrait/quote, names and parenthesized descriptors, selected row, buttons, name/gender prompt, Customize Your Faction, keyboard and cancellation. Chosen values must reach live faction/player state and saves. |
| First Base name | Starting Base creation supplies an automatic name; the original`#FIRSTBASE` prompt and its lower-console-connected backdrop are absent. | Show the real initial local-human naming flow at its original command boundary, preserve the suggested name, and persist accepted/cancelled behavior without replaying startup or consuming extra gameplay RNG. Match original`#NEWBASE` separately for later founding. |
| Terrain seams | The supplied Morgan Industries crop has visible black cracks along adjoining terrain geometry. | Reproduce on the same saved/generated terrain, repair shared-edge geometry/rendering, and check ordinary slopes, coast, wrap seams and camera movement. Do not hide the defect with a black mask or alter elevations to flatten it away. |
| Main-map unexplored terrain | The minimap consumes exploration. The ordinary main-map presentation does not mask unknown terrain. The opt-in native inspector deliberately reveals terrain and is not a normal-game fog implementation. | Render original unknown-area black/grid boundaries from authoritative observer knowledge; distinguish exploration from currently visible entities and remembered improvements. Movement/sight, load and faction focus must update all applicable layers without revealing hidden Units/Bases. |
| Guided tour | User explicitly accepts leaving it unimplemented. | Not a1.0 blocker; do not spend implementation time on the guided tour. Help/Datalinks and ordinary functional dialogs remain in scope. |

## First correction: blue setup artwork

SMAC setup pages now request the same source-backed FLC/common-palette blue
compositor as the title, with their actual row count. Heading labels are
separate from action-label selection. The artwork is explicitly bottom-aligned
inside the heading area; an actual render caught and corrected that alignment.
SMAX setup retains its existing route. This fixes the reported green fallback;
it is not a full setup-screen or font-matching claim.

The title transition/history test and ruleset, navigation and faction-selection
GSE scripts pass. Real1024x768 hidden-SDL/OpenGL runs exercised Start, ruleset
selection, map choice, size choice, Back and normal Exit. Final captures are
unaltered framebuffer pixels (PPM converted to PNG). Both capture processes
exited0 with no tracked OpenGL/memory leaks. The native executable is the cached
validated gameplay build; current GSE sources are loaded directly from this
worktree. No platform matrix was repeated for script/layout changes.

![Actual SMAC map choice with blue artwork](https://raw.githubusercontent.com/jhurliman/glsmac/302ceb399bee594f22d49507d02b5e3c9ceaf0d0/smac-setup-blue-20261020/map-choice.png)

![Actual headed SMAC size menu after alignment correction](https://raw.githubusercontent.com/jhurliman/glsmac/302ceb399bee594f22d49507d02b5e3c9ceaf0d0/smac-setup-blue-20261020/planet-size.png)

## Authored landmark geometry correction

The ordinary backend geometry cache omitted the bounded negative-detail
compatibility path already enabled by the native view renderer. Consequently,
authored Planet landmarks could fall back to legacy fans beside native fans.
The backend now enables the same measured mapping tables; unsupported negative
rows still remain unavailable. It does not flatten elevations or mask cracks.

A real load of the same Gaia2101 campaign (SHA256
`f5baf09e4002d22ef467140f57c8dc2d37e807773b2a30607076263d5c4910c3`)
at the same default camera removes the reproduced large black gap. In logical
ROI[72,210,415,330],3994 of4000 previously black pixels become terrain; six
black pixels remain. This is a specific repaired scene, not proof of all seam
or whole-map pixel parity. Before is the exact1024x768 crop from the canonical
1280x960 capture; after is a native1024x768 framebuffer capture. Neither image
has terrain edits. Normal load and Quit completed, exit0, no tracked GL leaks.

Validation:32 affected TerrainRenderUpdate groups, including exact negative
Pholus display/picking heights45/30/50/30/50; changed production analysis0
findings. The existing original Planet geometry verifier passes under
ASan/UBSan:6400 projectors,32000 heights,32000 shades,508 primary triangles.

![Before: mixed geometry leaves a black gap](https://raw.githubusercontent.com/jhurliman/glsmac/9b59770386c30f1a11bc2ebec619bb63b94bb0ab/native-landmark-geometry/before-logical.png)

![After: same campaign and camera with authored geometry](https://raw.githubusercontent.com/jhurliman/glsmac/9b59770386c30f1a11bc2ebec619bb63b94bb0ab/native-landmark-geometry/gameplay.png)
