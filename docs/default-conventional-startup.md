# Local SMAC conventional startup

The default local SMAC world now queues its existing HQ followed by an actual
starting conventional Unit. The queued event runs in the owning Start
accumulator: raw turn-zero creation preserves the selected design and stored
morale, and the existing first-turn seal supplies refreshed statistics before
frontend publication. The world script reads the admitted owner's
`starting_scout`; builtin design 2 uses initialized Infantry, while the selected
owned Speeder uses the Spartan Rover descriptor. It does not derive a design
from a transport slot. The native startup API authenticates that selection.

`Player.is_single_player` exposes the existing Single role. Both definition and
world activation require SMAC plus this local role; SMAX/network retain their
existing native placeholder path. The HQ remains on the legacy Base economy
path. This change does not activate canonical growth or worker allocation.

The provisional GLSMAC placement policy requires ordinary land, an ordinary
adjacent exit, and a minimum center distance of four tile-distance units.
The terrain-only Tile predicate accepts known rockiness, river, and the already
supported road/tube/forest improvements; fungus and interacting landmarks are
excluded. It is not Unit command authorization. Existing movement validation
still checks ownership, occupancy and other command inputs. Placement preserves
terrain and reports failure when it cannot find space; it does not clear fungus
or claim original starting-site balancing.

The explicit port camera policy uses facing 2, zoom 0 and ratio 1:1. The full
256-square texture retains the legacy sprite pixel scale (world size 2.56 by
2.7306667), pivoted at the nominal raster/model origin (128,128). Cached generated
facing-2 images were inspected: initialized Infantry occupies x113..135/y86..127,
and Rover x105..152/y105..140. The separate target-blit metadata (50,50) is not the
texture pivot. No original final-screen scale/placement parity is asserted.
The conventional morale labels are the seven installed MORALE names.

Focused validation reused the cached native build. All ten tracked groups
matching `Early conventional|Default conventional` passed: the original eight
claimed-Start/cancellation/restore cases plus two actual default-script cases
for local Gaia and Sparta. The new cases load the definition, world and spawn
handlers, observe raw-before-seal ordering, move the newly created Unit away
from its HQ through the real `move_unit` event, advance through the actual
turn-2 handler, and restore exact Unit records without replay. Unsupported
fixture terrain remains unchanged. The synthetic map/profile and asset-free
frontend do not claim a fully rendered default campaign or seven-player run.
Targeted LLVM15 analysis of Player.cpp and Tile.cpp passed (2 TUs, 3.23s).
