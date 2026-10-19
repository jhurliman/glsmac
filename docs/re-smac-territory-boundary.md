# Native source/execution map: territory-boundary CLUT and stores

Pinned terran.exe SHA256:
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
Public behavior/territory-boundary.md; territory_boundary_reference.py and cases.

## Mask and CLUT construction

Full45d830 initialization resumes from earlier coast boundary45de6b through
explicit pre-instruction stop45df34. Extraction5fc7d0 args source7c9c48,
key265,x2,y254,w60,h60,flags0. Nine literal source colors45de86..45deae:
d3,d0,29,8f,74,97,ce,5f,c0. Lookup256zero at45deb8..45dec6;
sourcecolor+10→class0..8 at45dec8..45dee0. Temporary fill9 through5f8950,
mapped copy602ba0,60×60 extraction634010 receiver782348 at45df2f.
Generated maskSHA83f4d24e204aaa95e7a67f3993d2e7c1069fb64d49db77947c175788771a7e77.

Global setup429e30 defaults8colors/opacity80 then SYSTEM boundarypercents
section616670/616690/616860 at42a35d..42a38c; native PALETTE.PCX load42a38e
then eight5fb010 samples: x580+8*i,y56 at42a3a8..42a3d1.
Cold generation42a82f..42a889:618250(target,row,10,236,100-alpha,alpha),
8rows at70f818..710018; exact copy into declared cache output buffer.
Our control supplies actual external SYSTEM field values, executes native PCX
load/sample and complete native8-row generation/copy fragment, not the text
parser or entire429e30 file-cache wrapper. Globalcolors249,241,252,255,251,65,102,235;
opacity0,50,60,50,50,60,50,70. SHA2048bytes
81822f3581485dcfe250c3a501d57984f667572d4b867c28288218329869508a.

Per-faction4297e0 loads factionPCX from saved faction filename; samples161747
and161757 at429982..4299b4. Transparency9 checks4299b9, laterothercolorsampling
has its own guards. Opacity conversion4299c5..4299f9: subtract10 inAL,
zeroextend, multiply100, signed division255 via magic80808081. Native target
sample and this bounded conversion execute for seven actual files; complete
618250 then constructs each row. Actual429c48..429c72 generator call and
warm cache429c0e..429c20 source-prove row destination/parameters; wrappers
themselves are not executed end-to-end here. Declared assignment owners1..7=
GAIANS,HIVE,UNIV,MORGAN,SPARTANS,BELIEVE,PEACE. Active rows match initial
rows in this stock assignment; both2048-byte banks independently checked.

618250 initializes outside[10,246) identity, integer mixed RGB and6186d0
common-only nearest search with first-tie retention, then restores configured
protected ranges. Stock6174d0 software profile has none. Prior effect CLUT
receipt already source/execution-proves numeric duplicate-palette behavior;
this receipt uses actual current Gamma100 palette, SHA1024bytes
c7129c7433e5d3028e4e3d7990e696fd9a5b321ebf24694cee6316e5d9a32ba6.

## Caller and actual stores

Flag10000→EBP-c0 at470931..47093e. Ordinary boundary471a00..471c57:
center visibility/capability/scenario gates471a0e..471a65; override EBP-18=0;
4f97a0 center return>=0. Helper signed owner byte7 at4f97c6;<=0→-1;
same viewer returnsdirectly; foreign recognition requires relation08000008,
or scenario80 bypass. Existing territory producer receipts cover real query
authority; this probe executes helper on each controlled center/neighbor.
Same bandcategory and byte3 region checks471b6e..471b9b precede neighborquery.
Eight offsets690ca8/690ccc. Different result builds10-byte mapping atEBP-4c8,
entries owner+1, exact odd/even/zero rules471bb8..471bff. Anydifference
emits even when selected classifications have no source pixels.

638e40 sixargs target,outline7f0b20,map,70f818,4,UV. Source782348 is60×60;
UV0,0,59,0,59,59,0,59. It calls63a590 edgehelper at638fb4/638fd0/6390a3/
639109 with zero shade, preserving half-texel bias. An initial portable
assumption of63f680/unbiasedUV was rejected by native span comparison and
corrected before freezing. No lighting is applied by the CLUT pixel loop.
At63932e capture source[EBX+ESI], mapping[class], existing[EDI]; selector0
skips. Otherwise639349..639363 selects(row=selector-1,destinationIndex).
Store639365 writesDL. New observer checksDL against expected CLUT value and
records every actual store, including identity-valued stores.

Native pipeline order Forest/Fungus→territory→coast→River. Optional Road/Tube
overlay47235b..47263d, flag20, draw636990 at472638 remains a distinct next
normal material investigation, not covered by this profile. No road/fog/
landmark expansion was performed.

## Receipts and scope

probe_territory_boundary_commands.py:31complete46fd00returns,154commands.
Uses new independent territory-boundary-private-20261019; no prior private
mask directory or frozen archive overwritten. Fixes the six-argument sink
only in this new control. Rows/PCX/raster calculations are native, not supplied
results. Text values, faction assignment and cache-buffer backing are explicit
inputs. First development global sample coordinate was transposed; corrected
580+8*i,56 native samples replace that unfrozen run. Earlier source archives
retain their historical scope and corrected documentation links.

probe_territory_boundary_raster.py:43frames (31plans+4clips+8owner-gradient
diagnostics),5024spans,265448attempts,97028stores. Attempt packing<HHIhBBBB>
is x,y,sourceOffset,zeroShade,class,existingDest,selector,willStore forowner mode;
earlier modes retain their own established field meanings. Store<HHB>.
All initial/active CLUT bytes, mask bytes, named-field commands, contours,
spans,attempt/store streams and indexed frames match independent reference.
Eightgradientframes each cover246distinct destination values at admitted
stores, including all20reserved; do not call this256distinct per-row samples.

check_territory_boundary.py and portable --asset-directory CLI both pass.
Owned palette/mask/indexed/art bytes are private ignored evidence only.
