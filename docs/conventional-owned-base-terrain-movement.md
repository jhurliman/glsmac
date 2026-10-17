# Ordinary nonfungus movement around owned Bases

The existing local conventional movement event now uses its ordinary land
edge-cost rules when owned Bases are nearby or occupy an endpoint. Flatness,
matching endpoint infrastructure, population one and a Headquarters facility
are no longer extra admission conditions for nonfungus movement. This supersedes
the corresponding narrow policy in the earlier
[remote-ground checkpoint](conventional-remote-ordinary-ground.md).

The current healthy Infantry/selected Spartan Rover, Q3, checked current stats,
canonical local controller and saved no-project capability boundaries remain.
The destination must be empty and the source unstacked; locks, animation,
foreign Units and possible enemy zones of control still reject. Canonical Base
manager/center links are checked before edge pricing. Endpoint or adjacent
foreign Bases remain outside this owned-Base scope. Owned Base population and
facilities do not feed ordinary edge pricing. Actual endpoint Base bits still
supply the existing road/tube discounts.

Source or destination fungus keeps the previous endpoint-HQ admission and
nearest-distance behavior. No encounter, combat, sight/exploration, healing,
network, snapshot or default-startup behavior is added. The copied-RNG
preparation and existing allocation-before-commit path are unchanged.

`fixtures/OwnedBaseTerrainMovement.h` projects all 18 ordered literal cases in
RE-SMAC-BASE-TERRAIN-MOVE-001, public fixture SHA256
`443556e58378baf6cf30e4f515195a2bd6e9163e5b476eb7598c5894715ac53f`.
For actual Gaian Infantry and selected Spartan Rover, the real event test checks
near/far rocky and forest edges, diagonal connected river, road to unroaded
forest, and departure from a population-seven Base with no HQ. Spent values are
six or one from initial zero; the Scout may overspend its allowance of three.
Shared seed 12345 and Base records remain unchanged. Membership assertions use
actual port identities rather than original linked-list indices.

Additional port assertions preserve CRT/JSF state, the prior nearest-distance
value, terrain and IDs, while actual relocation records placement and queues
one Unit refresh. These are not extra original-runtime observations. The test
uses real Unit/Base managers and the event path with the existing synthetic
map/runtime fixture; it does not claim a complete UI or sight-update replay.

Older terrain, infrastructure and owned-proximity negative rows are removed;
the full ordinary receipt replaces those assertions. Four population/facility
rejections now explicitly use destination fungus, preserving that separate
boundary. One older remote-forest rejection now checks distant fungus instead.
Other owner, domain, state, stack and unsupported-effect rejections remain.

The fixture was projected from the pinned public JSON and the diff passes
whitespace checks. Runtime validation is pending the combined cached build;
no separate build or platform run was performed for this source checkpoint.
Suggested focused selector: `conventional.*(movement|ground|HQ|terrain)|movement history`.
