# Native source/execution map: Road/Tube

Pinned terran.exe SHA256:
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
Public behavior/ordinary-road-tube.md, reference and cases beside it.

472355..47235e stores flags&20 inEBP-38. Center4f9320 at47236c requires
live marker+8bit1 and matching Base coordinates in96f1b0/count959fd4.
472377..4723ae admits owner96f1b4==viewer or visibility96f1ba&(1<<viewer);
foreign hidden becomes−1. Tile visibility/territory is a separate authority.
4723b1..4723c7 requires land and(selecteditems4 orBase>=0). 4723cd..4723d8
sets Tube mode EBP-2c when selected8 orBase>=0. Tube-only8 doesnotadmit.

Eight-neighbor loop4723eb..4725bd uses690ca8/690ccc, singleXwrap unlessflat1,
Ybounds, and band>=3. Art indexESI=(direction+2)&7. Selectedneighboritems at
472492..4724b0: viewer>=0 reads tile+12+4*viewer, otherwise live+8.
Native4f9320 at4724b5 plus same Base owner/visibility rules qualify neighbor.
Requires selected4 orBase at4724f2..4724fc; EBP-88 recordsanyConnection.
Tube ifcenterTube and(neighbor8 orBase), elseRoad. Tube bank7845d8+112*index,
call636990 at472561, setsEBP-34 TubeConnected. Road bank7849c8+112*index,
call636990 at47259d. Quads emitted in native neighbor order.

Fallback4725c3..472638: centerBase skipsall. TubeMode&&!TubeConnected chooses
isolatedTube784568; otherwise noConnection chooses isolatedRoad784958.
Outline7f0b20,shadeEBP-b0,UV0,0,55,0,55,55,0,55,count4. Roadcrops form
3×3grid at775395 with57stride; Tube at775566. Fullnative45d830 extraction
executes. All61contexts capture key9 at47235b. Existing636990 compare636eb1,
shadeadd636eb5,store636eb7, and63a590 UVbias apply; no new shader variant.

probe_road_tube_commands.py executes46fd00 entry to explicit pre-instruction
47263d afterlinks/fallback, NOT full renderer return. Subsequent Base bodies
and otherobjects excluded. Actual Base array/live marker/visibility supplied,
no Base-query return supplied. Prefix omits renderer cleanup, so independent
cases explicitly restore entry scalarbounds−256/+256 before rendering; land
then sets−4/+4. First unfrozen development run lacked that reset for sea;
corrected before accepted check/freeze. Old evidence was not overwritten.

probe_road_tube_raster.py composes accepted opaque/depth/coast/keyed-unlit/
keyed-shaded native observers with new road-tube-private-20261019 output.
61contexts335commands match independent named-field selection/geometry/
contours. Native65frames (61+4clips) match10364spans,360796attempts,
169846stores andfullindexed hashes. check_road_tube.py and portable CLI pass
using external installed PCX. No owned art in public fixtures.
Road+coast/River/Forest/Fungus dynamically covered; Road+territory order is
source-proved rather than another composed frame. Later branches stay separate.
