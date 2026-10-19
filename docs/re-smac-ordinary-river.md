# Native source/execution map: ordinary River

Pinned executable SHA256:
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
Public contract behavior/ordinary-river.md, reference river_reference.py.

## Correct branch identity

471a00..471c57 is not River: renderer flag10000 gate comes from470931..47093e.
It calls4f97a0(viewer,x,y,0,0). That helper reads signed retained tile byte7
at4f97c6, rejects nonpositive identity, returns that identity at4f984a,
with viewer/relation admission at4f97da..4f9829. Earlier territory-cache
contracts identify byte7 as territory owner. Caller requires same retained
region byte3 and land/sea class before comparing differing returned owners.
Mapping entries contain centerOwner+1. 638e40 receives six arguments:
target,vertices,mapping,CLUT,count,UV. Source bank782348,CLUT70f818.
At63932e..639365 it selects mapping[sourceClass], skips0, otherwise uses
CLUT[(selector-1)*256+existingDestIndex]. This is owner-color boundary
remapping, not River art. CLUT construction and dedicated native compositor
controls are still the precise open follow-on. Prior water fixture's unused
638e40 sink ABI was wrong; these River controls leave flag10000 off and do
not rely on that sink or claim to fix the old frozen fixture.

## Actual River producer

471db3..471de9: renderer ah&4 means400; live center items+8 bit80;
sea boolean EBP-14 must0. Four offsets690c80/690c94 read as
(1,-1),(1,1),(-1,1),(-1,-1). 471e06..471e76 wraps/clips coordinates,
tests neighbor live+8 bit80, and ORs1<<neighborIndex. No neighbor band test.
471e78..471e9e creates UV0,0,55,0,55,55,0,55 and modeFFFFFFFF when
ordinary visible EBP-1c is true. 471eab uses EBP-b0 outer shade array;
471ebd uses outline7f0b20. Receiver783e68+112*mask;63d6b0 at471ed6.
Native45d830 atlas setup records16crops at45db71 starting280259,57stride.
471ef5..471f17 tests live item200, then draws receiver783e68 again.
This raw conditional's semantic name is deliberately not newly assigned.

River is after sea/land base, Forest/Fungus471833..4719fb, optional territory
branch471a00..471c57, and land coast471c5c..471dad. River center sea rejects
before any art. Flags1 hidden and remembered/live disagreement controls isolate
this from earlier feature gates. Later optional object overlays remain outside.

63d6b0 FFFFFFFF delegates636990 keyed shaded, with existing63a590 half-texel
edge initialization. Compare raw key636eb1, shade add636eb5, store636eb7.
Key9 precedes shading; ordinary scalar limits -4,+4. Source key and values
remain palette indices, not RGB. Reused native unlit/opaque/depth/coast raster
paths execute for earlier composed commands as in their closed receipts.

## Evidence

probe_river_commands.py:33complete46fd00 returns,172ordered sink captures;
all16masks,4shoreneighbors,flags1/noitem/sea/remembered-only/live-only,
extra200,Forest/Fungus,raisedcenter,westwrap/flat,bothpoles.
probe_river_raster.py:37frames including4additional clipped replays;
5103spans,163184attempts,101069actual stores. New private river-private-20261019
holds generated masks, indexed targets and attempt/store streams. The observer
records actual636eb1/636eb7 in addition to earlier variant sample/store sites.
check_river.py independently reproduces named-field commands and all raster
spans/hashes from external installed PCX files. Public cases contain no art.

During development the first raster observer missed the new keyed-shaded
sample/store hooks; that run had valid final indexed frames but incomplete
streams. It was replaced before freezing; the corrected run observes all
variants and matches portable results. The west clipping replay was also
corrected to intersect the camera-centered tile. No earlier archive changed.
