# Native owner0 badge raster port

`DecodeSmacNativeBadge` preserves indexed UNITS.PCX source pixels through
byte-offset10 wrapping, original6/7 preprocessing, owner0 marker remap and
key9 transparency. Gamma100 palette construction includes the20 reserved
software entries; index65 black stays opaque. No health-bar hole is cut.
The frozen original recipe and limitations are in
[native-badge-raster.md](native-badge-raster.md).

The SDL2 loader decodes both source assets once per resolved path pair and
caches all14 small results. Each requested texture has independent ownership;
its caller may retire or edit it without changing another result. Existing
opaque menu palette conversion and ordinary local-PCX decoding are unchanged.

Validation: the opt-in installed-asset probe matches all14 indexed and all14
canonical RGBA SHA256 outputs from the original software renderer. The native
GSE test exercises the production SDL2 loader with padded indexed rows, equal
local RGB for distinct markers, wrapped/system entries, opaque black, common
colors, complete bank caching and independent owned textures. Focused C++
correctness analysis and ASan/UBSan cover the loader and its native renderer
consumers; leak detection is disabled. No full map/body screenshot parity or
non100 gamma result is claimed.

To repeat the installed comparison, compile `tools/native_badge_probe.cpp`
against SDL2 and SDL2_image, then run `tools/check_native_badges.py` with
three positional arguments: probe executable, purchased asset directory and
output directory. The verifier checks the pinned asset hashes first. No
proprietary asset pixels are stored in this repository.
