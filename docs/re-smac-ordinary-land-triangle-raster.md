# Native land triangle shader execution

Pinned EXE SHA2560a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01. Native5f0be0(TEXTURE.PCX,0,10,236) rebases the entire selected crop by+10 (independent raw PCX decoder plus10 matches the literal native crop hash); plus634010(source,1,58,56,56,0) initializes7866a8, matching45d984..45d998. Four controlled worlds execute47dc00,46f3b0,47cfc0 and46f930, then46fd00 entry through47102f. Actual470c5f calls63dcc0 with flagsFFFFFFFF; its sentinel delegates635af0. Native635af0 and63a590 fully execute, no raster/pixel result sinks.

Capture63dcc0 triangle vertices/light/UV;635af0 confirms full path. Capture63600d before actual store: EDI destination, EBX+ESI wrapped32-bit source address, AL final index, CH shade integer. Every write satisfies final=(source+signedCH)&255. Native globals9c4478/9c44bc give initial16.16 U/V;9c4464/9c4448 derivatives;CX lowword shade8.8;9c44ac step. Independent replay matches8,967 source addresses/phases/results and four literal frame hashes.

63a590 starts U/V with8000 bias, uses642890 signed fixed-point division, and quotient/remainder X stepping.635af0 sets x87 rounding toward zero before derivative FISTP;635e13..635e37 writes16.16 U/V and8.8 shade derivatives.636003 ADD AL,CH is palette index addition. No per-pixel CLUT appears on this known-land path. Alternate63dcc0 stipple path and other texture/detail shaders are explicitly excluded.

Fixed camera40,40/zoom0/nooverride; native target software bitmap1024x511 initialized with diagnostic250. Supplied Win32 bitmap/heap services inherited from existing overlay harness. No whole-window presentation or later shoreline overlays. Public reference is exact native-span replay, not independently implemented vertex-to-span conversion.

Reproduce probe_land_triangle_raster.py; finalize_land_triangle_raster.py verifies all per-pixel states, builds public fixtures/reference handoff and freezes evidence in tmp/reference/ordinary-land-triangle-raster-evidence-20261019.
