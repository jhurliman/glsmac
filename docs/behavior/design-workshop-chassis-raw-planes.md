# Causal chassis controls: raw indexed color and depth expectations

2026-10-21. Original terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Public hashes and scalar expectations](design-workshop-chassis-raw-planes-cases.json).
This adds only the two existing [shared-position causal states](design-workshop-shared-position.md).
No port-renderer output is an input to these expectations.

Capture native **631613**, immediately after654770 returns, before the
keyed copy into the card or renderer cleanup. Each color plane is exactly
**256×256 unsigned8**, row-major, pitch256, **65536 bytes**. Each depth plane
is exactly **256×256 unsigned16 little-endian**, row-major, pitch256 samples,
**131072 bytes**, without headers or padding. Initial color is index9 and
initial depth is65535 throughout. These buffers contain no label/background
pixels from the99×85 card crop.

| State | Color SHA256 | Depth LE-u16 SHA256 |
| --- | --- | --- |
| zero_only | `25546750d0e1105189e0de82f8cdf2d73ae3291acdb196ae2e4d43d52d2bdffa` | `0974b4ddec8cd91866faf5019e4fb6eb84a64d27c624912e11706b75b3230643` |
| retained_main | `4fa3a6e662914027a11cd87c912ccda8bb17382c734fc0d3847a650997f9c7d7` | `1ba6ef5cd0a8a8e24b7c8e402b956316cc469dab3d29b1dd16fedd795207692b` |

**Both states: draw_count=64, processed_records=42810.**
Correction:42874 is the inclusive kernel-count operand sum, with64 ignored
terminal records. See the [native loop audit](design-workshop-kernel-count-semantics.md). Draw count means
native raster **kernel/group calls**, here all6867f0, corresponding to the
one-group raster-draw counter. There are separately58 entries to geometry
function659800;58 is not the group draw_count. The requested count operands sum to42874; native bit20 decrements each
by one before executing its body, giving42810 processed records. Both
requested and effective64-element count lists are in the JSON. Neither count means final visible pixels or successful
sample writes. Final non-key color/nonclear depth cells are122/122 at zero
and116/116 at retained position; do not use these as written_samples.

Fixture: owner1, raw Scout2, chassis0/weapon0/armor0/reactor1/no abilities,
phase0, x87 control037f, original43de60, native draw flags2, scale word3d4ccccd.
Positions are zero, or retained words `40d263c0,425f4084,c37c0289`.
Parent anchor is(79,47), parent clip(30,7)-(129,92), translating to native
internal clip(79,88)-(178,173). Actual rotation and light words are in the JSON.
No new yaw, technology, design or material matrix is introduced.

The probe rebuilds native descriptors from owned assets and restores only
the captured scalar scene prefix, retaining this guest's allocation pointers.
The complete scalar prefix2c..10b at631370 matches its earlier causal state.
After capturing the raw planes, the original keyed copy and caption paint
continue: **all8415 bytes of each resulting99×85 card crop match the earlier
causal receipt exactly**. This checks that the smaller raw-only replay
reproduces the same consumed state without needing another full-screen matrix.

Original raw planes, scene snapshots, kernel arguments, native source and
probe scripts stay private in
`tmp/reference/independent-workshop-chassis-raw-planes-evidence-20261021/`.
Only hashes/scalars are public. Earlier frozen packets remain unchanged.
