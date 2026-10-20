# Shared owned terrain authority

`SmacTerrainAuthority` provides an immutable terrain view for installed and
generated sources without equating raw terrain availability with installed-map
provenance. It reuses the existing 44-byte cell and field readers. Generated
records never pass through an installed-file decoder or receive a manufactured
Planet header.

The installed factory takes a map value and owns an immutable container. The
generated factory accepts only the completed pre-startup producer boundary,
copies every raw record and retains actual dimensions, topology, resource
pattern, sea/climate offsets, resolved categories and generation rule flags.
Coordinates require matching row parity and bounds. The generated authority
retains the original seed-1 plane hash after its producer is modified and then
destroyed.

Metadata categories follow generator order: size, ocean, land, erosion,
temperature, rainfall, life. Installed decoding only populates the categories
already established by its adapter; land/erosion and generation primary rules
remain explicitly absent. Unknown header bytes remain in the original container
and survive re-encoding. Missing metadata is not a license to choose defaults.

## First live consumer bridge

`GlsmacResourceMapPolicy` now queries altitude, landmark bonuses, resource bonuses
and site signals through the common view. Its installed factories establish
the original container and common view together. Climate, flora, starting
balance and bloom publication rebuild both through the same source-setting
operation. Existing installed-pointer provenance checks remain explicit while
their callers are migrated.

The original policy wire formats are unchanged. `RecordBytes()` exports only
the complete raw plane; it is not an installed-map or campaign codec. No
generated-policy factory, generated save format, live generated-world
publication or post-startup mutation adapter is introduced here. The new
generated snapshot is a detached startup input, not a playable-world claim.

Regions, landmark labels/profiles and gameplay RNG retain their existing
separate owners; this class does not create competing mutable copies of those
authorities. Cross-queue consumers retain shared ownership of the terrain
snapshot rather than borrowing a temporary producer's record vector.

## Verification

The generated test runs the genuine complete terrain producer, compares every
record and all metadata, then mutates and destroys the producer. The retained
snapshot still hashes to the original pre-startup plane
`17095ab34ed6a22f2e47429aa14d7eb0bb5d40001c6b21e9641d41cad9c733f9`.
Partial generation and invalid coordinates are rejected.

Installed tests preserve poisoned unowned header/record/trailer bytes, known
signed metadata and an explicit byte-for-byte old policy envelope. Retiring a
policy leaves its queued snapshot alive. Live flora and climate publication
checks compare the common view with the updated original records/metadata and
verify that earlier sources remain unchanged.

Validation: cached incremental build; 37 focused native groups covering Planet,
resources/sites and the producer; relevant publication assertions rechecked;
correctness analysis of nine production TUs; focused five-TU ASan/UBSan run.
All pass. The shared policy layout required a broader incremental rebuild; no
new checkout or local platform matrix was used.

Next are the remaining raw-authority consumers, startup, generated mutation and
save/network payloads from the [integration plan](native-generated-world-integration.md).
Gameplay still uses the prior procedural generator.
