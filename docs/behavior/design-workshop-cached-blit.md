# Isolated cached sprite/background blit receipt

2026-10-21. Original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The [scalar receipt](design-workshop-cached-blit-cases.json) separates the
cached sprite from later native captions/data. It retains the exact5fe680
entry and return57015f boundaries, before57015f executes its first instruction.
An independent scalar replication/shadow loop matches every destination byte.
No port renderer or desktop image is an expectation input.

Private immutable bundle:
`tmp/reference/independent-workshop-cached-blit-evidence-20261021/`.
Manifest SHA-256:
`a94d992288e88544283f593ef5db54d7638cb445150657b445e69a719e52acd4`.
There are42 payloads.

The initialized lookup below is an explicitly harness-selected **037f**
profile. "Preferred comparator" refers to this isolated compositor fixture,
not a recommendation to replace the runtime's earlier027f initializer profile.
See the [precision/cache-lifetime boundary](design-workshop-shadow-precision-lifetime.md).

## Preferred comparator fixture

Use these named binary files from that private bundle. The two destination
images are **311x311 indexed8, pitch311**, with no caption/data painting.

| File | Meaning | SHA-256 |
| --- | --- | --- |
|`initialized-before-clip311.indexed.bin`|Destination before the sprite|`e972fad227b9752631de74f08f56a21db259d28cc40b6628e0b748967095a0ef`|
|`initialized-after-sprite-clip311.indexed.bin`|Expected destination after the sprite|`b0a21c378fd7b95920ecdf4d2b67d60e4a648c7ddab3ac926a2b6842d7e95d20`|
|`sprite87x67.indexed.bin`|Trimmed sprite,87x67, pitch87|`9c784a21e0e3f0d902d93dcfc39eaee66238e2cb70127a33570736992bd55c42`|
|`initialized-shadow-lookup256.bin`|Original palette-produced shadow lookup|`0229080d2b3b2e56a39523f3ced54fcbcbe8bc8dea8a8548d5fbe75820cd6ef3`|

Seed the comparator destination with the before bytes. Use key9, shadow source
index0, scale30/16, and sprite stored trim offset(8,0). The original cell-call
anchor is **(62,85)** relative to the311x311 destination. The scaled trimmed
raster begins at **(77,85)** and has dimensions163x125. Clip to[0,0,311,311).
Do not recenter the87x67 trimmed extent or replace the destination background
with a solid fill. In original client coordinates, add(140,9): call(202,94),
trimmed raster origin(217,94), preview clip[140,9,451,320).

The source bytes are independently decoded from the player's UNITS.PCX cell
(2,2,100,75), with loaded index offset10, loaded6/7 replaced by9, and trim
[8,0,95,67). This is Scout selected2 with an explicit icon0 override; installed
Scout's icon is255/-1. It does not change the installed-metadata contract.

## Palette initialization boundary

The first complete main-paint capture exposed an all-zero shadow table in the
offscreen bootstrap. Its pre/post clip and zero lookup are retained as an
additional diagnostic: `before-clip311.indexed.bin`,
`after-sprite-clip311.indexed.bin`, and `shadow-lookup256.bin`.
The zero-lookup after hash is
`ddc79c83082d712d6209fbdebdd2ec38383edf4e66ce516fb3281c002b4b274f`.
That is not the preferred initialized-palette comparator above.

Source429e30@42a042..42a055 initializes71db18 on the cache-miss path through
`617fa0(table=71db18,start=0,count=256,adjustment=-43)`, with palette959aa8
as this. The initialized control executes that complete original producer
against the established Gamma100 palette input, SHA-256
`c7129c7433e5d3028e4e3d7990e696fd9a5b321ebf24694cee6316e5d9a32ba6`.
It produces102 distinct lookup values. Then it executes the complete original
5fe680 using the retained sprite, destination and call operands. Canvas
admission/flush are supplied boundaries; RECT intersection executes a scalar
equivalent. Both original calls return0. This proves the producer-plus-blit
at those admitted stages; it does not replay the entire palette/cache lifecycle.

The initialized after image changes8,668 destination pixels and matches the
independent replay exactly. Both controls leave pixels outside the sprite
destination unchanged. The saved full623x387/pitch624 before/after buffers
are also retained for boundary inspection; use the311x311 files for the port
sprite/background comparator.

## Upscaled shadow sampling edge

At scale30/16,604070 supplies the original replication counts;603970/603a20
trim their final counts to the truncated scaled extent.5fe680's enlarged
sprite path resolves a source0 shadow sample **once**, using the destination
at that source pixel's replication-block top-left. It then replicates that
looked-up value across the whole block. See5ff238 and following branches.
It does not independently look up each destination pixel in the block.

The textured native background distinguishes these operations: independent
per-destination lookup disagrees with the initialized native result at
**2,320 pixels**, while replication of the first destination's result matches
every byte. The earlier zero lookup could not distinguish the alternatives.
Key9 skips the block; other source indices copy across it. These observations
are scoped to this enlargement control; they do not invent a general shrink
filter or alter the already checked sprite primitive's other branches.

The initial boundary capture's host diagnostic printer raised KeyError(args)
after the native main painter returned and its entry/return buffers were saved.
The checker admits only that specific formatting error and verifies those
buffers against independent replay. The corrected capture wrapper is retained;
the initialized producer/blit control completes cleanly and is the preferred
fixture. No guest execution failure is treated as a successful boundary.

The [distance/store follow-up](design-workshop-shadow-distance.md) explains
lookup input6 choosing178 over identical-RGB177. The native comparator is
strict< with an extended candidate and binary64-reloaded best; it is not a
general<= duplicate policy. Exact subtraction/square/add stores and an
independent arithmetic reference are now pinned there.
