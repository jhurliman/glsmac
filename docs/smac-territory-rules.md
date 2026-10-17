# Land and sea territory claims

`map::smac_territory::Rebuild` produces every tile's claim from supplied terrain,
region IDs, border distance and ordered Base records. It implements the common
integer distance, later-record tie selection and strict cutoff. Sea claims use
the halved cutoff and are cancelled by an equally close or closer foreign land
Base. A veto leaves the tile unclaimed; it does not transfer the claim or search
for another sea candidate.

`ReplaceClaims` composes this result with an existing owned
`SmacWorkerKnowledge`. It requires matching geometry and returns a new value
changing only the claimant fields. Exploration, observer relations, all-map
reveal, Unit observation IDs, visibility and convoy state are preserved. Empty
Base input clears all previous claims, including native claims. Invalid input
leaves the supplied knowledge unchanged. Existing knowledge serialization and
worker projection consume the result without a new format or wire version.

The admitted port domain is the existing doubled-coordinate worker grid:
even width 2..512, height 1..256, positive signed32 border, valid parity
coordinates, at most one Base per tile, playable owners 1..7, land regions
1..63 and sea regions 65..127. Terrain and region must agree. These are explicit
bounded inputs, not claims about original malformed-input recovery. Distance
arithmetic is widened before subtraction; all admitted coordinate products and
distances fit. The positive odd sea cutoff floors before comparison.

## Original evidence and tests

The producer follows [land territory](behavior/land-territory-producer.md) and
[sea territory](behavior/sea-territory-producer.md). The owned fixture contains
all 14 complete 64x32 maps: 14,336 literal original claim writes, including
unclaimed writes. Twelve subsequent recognition/worker frames compare the raw
and recognized 21-square claims and unavailable masks through the actual
knowledge replacement, existing codec and worker projection. Recognition is
supplied independently; these tests do not implement a diplomacy command.

`tools/reference/project_smac_territory.py` pins the two public fixtures, two
retained original reports and executable hash. It reads the actual ordered
claim writes and cross-checks their public coordinate projections. It verifies
the land selector's supplied region arguments and the coastal region counts.
There is no imported oracle or executable rerun. The compact committed fixture
contains outputs and synthetic coordinates, not executable or asset bytes.

The supplied region arrays are the actual outputs for the controlled all-land
and X>32 coastal worlds. They include the distinct polar IDs. They do not
implement the [water-region producer](behavior/water-region-rebuild-and-freshwater.md).
Synthetic tests separately check wrapped/nonwrapped distance, a distinct
supplied region, strict distance8 exclusion, border1 sea behavior, signed32
maximum border, late invalid input and owned non-claim preservation. These
diagnostics do not add original seam, native-owner or complex-ocean evidence.

## Live integration boundary

This checkpoint performs owned-value replacement only. It does not publish a
Map change, schedule a territory rebuild, create Bases, or relax mutation gates.
The next live caller needs three explicit inputs:

- Current `Tile` has an authoritative `is_water_tile`, but no region ID or
  region rebuild. A Map-owned GLSMAC topology can be generated from that actual
  classification and saved explicitly. Its equality/region-boundary semantics
  must be defined; assigning region1 to all land would merge disconnected
  regions. Original capacity-merging and region-table reset behavior are not
  implemented by this rule.
- `BaseManager` currently stores and serializes Bases by port ID in a
  `std::map`. `Base` can accept supplied IDs and restore gaps. This is not an
  established original current-record order or creation-age guarantee. The live
  adapter needs a declared stable order, preserved across restoration, before
  using it for ties. An explicit saved ordered Base-ID list is one option.
- The rule receives its border distance explicitly. The current frozen
  settings profiles do not expose that setting; a live caller must bind the
  actual configured value rather than silently choose the installed value 8.

Once those producers exist, the caller can stage claims alongside exploration
on one owned knowledge value, validate canonical Base/Unit bindings, and publish
the replacement at the existing no-callback commit boundary. Base creation,
removal and capture must invoke that same staged path. Restoration must retain
saved authority without replaying placement. Existing `GameWorkerKnowledge`
validation checks entity bindings, but it does not supply the missing topology
or territory record-order contract.

## Validation

Four exact native test helpers passed in an isolated Linux clang15 normal
build and ASan/UBSan/LSan build with leak detection and halt-on-error enabled.
The harness includes the actual native test TU and links the actual territory,
knowledge, frame, availability and collection production TUs. Targeted clang15
analysis of the new production TU reports zero diagnostics. The literal
projector was regenerated successfully. This is focused helper execution;
the full engine registration, allocation tracker and full platform suites were
not rerun for this checkpoint.

Two isolated copies of the production TU were compiled against the same test
helpers. Replacing later-record ties with first-record ties failed the recorded
land map; changing the sea veto from `<=` to `<` failed the recorded coastal
map. Both returned assertion failures, while intended source bytes were
unchanged.
