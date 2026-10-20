# Native generation region rebuilding

`RebuildRegions` implements original `0x5b4200` over owned 44-byte tile records
and complete 128-by-28-byte region storage. It is the region operation used by
contour morphing and generated-world rejection/retry decisions.

The implementation preserves the source's ordered breadth-first labels,
non-expanding polar rows, capacity merges into slots 63/127, and stale region
areas across calls. Reserved slots participate in the original merge scans.
Polar areas accumulate rather than being reset. Freshwater counts exclude the
component seed and update only the landmark bit, preserving other DWORD bits.
Land adjacency storage is cleared and rebuilt with the original neighbor skips;
sea adjacency and unowned record fields remain untouched.

Summary geometry belongs to the caller. Storage is initialized only when its
size changes; existing padding is retained. Odd widths use the original floor
row stride, including aliasing, rather than an adjusted rectangular stride.
Region rebuilding consumes no random numbers and leaves terrain dirty flags
unchanged. Progress callbacks cannot mutate or reenter either owned input.

## Original comparison

Thirteen original executable controls compare complete raw tile and region
planes, secondary flags, summary bytes, **67,424 ordered tile stores**,
**49,794 ordered region stores**, and **1,458 checkpoints**. Most controls rebuild
twice to test retained history. They cover land/sea capacity paths with zero and
high stale areas, all-land/all-sea/coastal worlds, freshwater all/sparse marks,
and a flat mosaic. Full region storage begins poisoned so clearing unrelated
fields cannot silently pass. The odd summary width preserves padding bytes.

Numeric input constructors reproduce the probe inputs and are checked against
original input hashes before each run. No purchased assets or captured
intermediate generation outputs are production inputs.

Source executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Frozen `native-pretemperature-generation-evidence-20261020` manifest SHA256:
`3ccbd8480e7ae54006bc663cb1336385046cf60e33d9897da2c097352d48c3d3`
(32 files verified before extraction). Compressed original numeric receipt
`generation-morph-regions-original-x86-20261020.json.z` SHA256:
`d2ee395a2ddc97886fd47267fa04cceb44810c231668edcf70cb5b0e132403dc`.

The cached incremental build, six focused native groups, production-TU
correctness analysis and focused three-TU ASan/UBSan run pass. The generator
still needs contour morphing and outer retry orchestration before replacing
live procedural generation. No gameplay rendering changes in this step.
