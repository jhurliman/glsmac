# RE-SMAC-PLANET-OCEAN-001: imported Planet ocean category

For the pinned original SMAC **Map of Planet** asset, the retained ocean-coverage
category consumed by restart site search is **1** (the setup label is 50–70%
ocean). This is an integer category, not a measured water fraction or a default
chosen by the importer. It is separate from the loaded **world-size category
4**. [Numeric cases](planet-ocean-category-cases.json).

## Decoder field and guards

Offsets are zero-based bytes from the start of the original file:

| Property | Value |
| --- | --- |
| Header start | **15** (`0x0F`) |
| Ocean category relative to header | **2612** (`0xA34`) |
| Ocean category absolute file offset | **2627** (`0xA43`) |
| Field width | **4 bytes**, ending before file offset 2631 |
| Scalar representation | **signed 32-bit little-endian integer** |
| Fixed header size for this profile | **2724 bytes** (`0xAA4`), ending before file offset 2739 |
| Normal category domain | **0..2 inclusive**; pinned installed Planet contains **1** |

The pinned container starts with `TERRANMAP`, a NUL and a following delimiter
byte (26 in this asset). Its four-byte little-endian signed version is at file
offset **11** and is **5**. This receipt admits that version-5 map layout; it
does not establish arbitrary newer layouts. The original map loader's version
gate rejects versions below 5 and has no upper-version check at that gate.
The header is a fixed-size read rather than a length obtained from a serialized
header-size field: require the complete 2724-byte header, hence at least 2739
file bytes through its end, before decoding this field. Full import still needs
the existing geometry/tile/coarse-data checks.

**Range validation is separate from native loader acceptance.** The loader
copies this word without a category-range check. Field-only mutations to 0, 2,
−1, 3 and 16909060 all load successfully and retain that exact signed value.
The last control stores bytes `[4,3,2,1]`, identifying the little-endian word at
the stated offset. These controls do not establish valid gameplay for malformed
categories. For the port's admitted normal-map state, validate 0..2; expect 1
for the pinned installed asset. Rejecting out-of-range values is an explicit
port admission guard, not an original loader rejection. Do not treat −1 as
unsigned 4294967295 or silently replace it with 1.

## Imported authority and lifetime

The authority is the **serialized map header**. The native map loader reads the
complete header, including the retained ocean setting, into active world state.
Prior values 0, 2, 77 and −1 are each replaced by the file's value 1. Thus an
abandoned generator choice does not survive a successful original Planet load;
nor does a negative prior choice trigger random ocean-category selection here.

The selected Map of Planet menu arm loads that file, runs the original terrain,
climate, river, region and site-cache preparation, then reaches the setup
controller with category **still 1**. Controlled native execution returns from
the complete preparation routine without any native write to the retained
category. The menu keeps map regeneration disabled on this path. The generic
generated-world random-category producer is a different path.

The port should publish the imported ocean category with the accepted Planet
world and retain it for the
[restart input lifetime](restart-input-lifetimes.md). Project the serialized
setting into its live restart-search authority; do not inherit the previous
generator's value, infer one from the number of water tiles, or substitute a
provisional fractional coverage. Later terrain/sea-level changes do not by
themselves establish a producer that recomputes this setting from actual water.

Evidence: four complete native loader runs with poisoned prior categories,
plus the actual selected menu-arm → native loader → complete native world
preparation → setup-entry boundary. Installed world-builder numeric tokens pass
through the original clamp/store code. File, heap, empty message-pump, clock and
unrelated presentation services are supplied; category results and terrain/
region producers are not. The clock is controlled for preparation's final
reseed; this receipt does not certify the complete startup RNG stream.

The setup controller's difficulty/rules/faction dialog and later faction
placement are not newly executed here. Their existing
[choice-transfer receipt](setup-wizard-choice-transfer.md) identifies the
no-regeneration continuation. This is one installed standard Planet profile,
not proof for Huge Planet, arbitrary maps, scenarios, SMAX or a full save/load
round trip.
