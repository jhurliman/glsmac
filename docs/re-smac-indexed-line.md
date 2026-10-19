# Native source and focused software-line store receipts

Pinned terran.exe SHA256
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
Public behavior/indexed-line.md, indexed_line_reference.py and cases.

Entry5fa3f0 bitmapECX,5args x0,y0,x1,y1,color; RET14hex. Bitmap+54 software
bits or+58hardware object required. Xequal5fa418 delegates5fa9f0 at5fa429;
Yequal5fa43f delegates5fa880 at5fa44a. Samepoint therefore reaches vertical
equal-Y reject5faa2a..5faa2c. Both axis helpers reject originalequal endpoints,
sort varying coordinate, clip left/top inclusive right/bottom exclusive, and
include bothdistinct originalendpoints. Clip-to-one still emitsone store.

General5fa458..5fa46b swaps bothendpointpairs whenx1<x0. Sourceclip+20..2c,
pitch+4a8,softwarecachedpointer+50,lockcount+6c. PositiveY branches5fa535
Xmajor(dx>=dy),5fa5e8Ymajor; negativeY uses nativeabs66270e at5fa6c2,
then5fa6ca Xmajor or5fa790Ymajor. Allstart error2minor-major and use JG
for minor advance, so error0 doesnotstepminor. Clippedprefix advances same
error rather than recomputing geometricclip endpoints. Reusedsource sequence
filteredto clip reproduces native per-pixel ordering for all accepted cases.

Stores:5fa598/5fa5da positiveXmajor;5fa645/5fa686 positiveYmajor;
5fa73c/5fa781 negativeXmajor;5fa7e9/5fa822 negativeYmajor. Rawcolorbyte
only, no key/palette/shade. Verticalstore5fab12 writesAH then adds pitch and
LOOP; helperRET10hex. Horizontal5fa994 calls actual661fe0 memset, whose
REP/byte/dword stores execute and are watched; helperRET10hex. Bitmap width80
and signednegativeheight84 are additionally checked by axishelpers.

Softwarelock path5fa478 publishes+54→+50 and increments6c;5fa82b cleanup
decrements and clears50/6c whenouterlockreleased. Axisdelegates have analogous
paths. No DirectDraw pointer/lock API supplied or tested. Startingpointer/lock
state explicit0; final0 verified. ReturnEAX isnot given semanticstatus.

probe_indexed_line.py uses bare pinnedPE machine, declared32×24/pitch40
softwarebuffer with rowpadding253,background17,32guardbytes either side.
No raster/clip/helper return replaced. UCmemorywrite observer recordsraw
address,width,value and normalizedlittle-endian<x,y,index> byte stores.
709complete nativecalls:700nonempty (includingoneunbacked),9emptyterminating.
3362stores,all guards/padding preserved, exactcallee stackcleanup andlockstate.
625grid endpointpairs plus namedreverse/tie/outside/clip/color controls.

Emptyheight diagnostic: clippedvertical crossing[2,6,14,6] sets firstY6,
lastY5, then count=last-first+1=0 at5fab02..5fab08.5fab12 stores before LOOP,
which would wrap0→ffffffff. Initial exploratory guard watcher exposed this;
accepted separatehazard hook stops at5fab12 withECX0 beforeany store. No
complete return orsafe no-op claimed. New portableprofile rejectsemptyclips.
This is a real source/store-loop boundary, not a hypothetical OS warning.

indexed-line-original-x86-20261019.json retainsrawaccessPCs; publicnumeric
cases omit those privateimplementation addresses and include allorderedstores
and fullstridedframe/store hashes. Syntheticdata only, noownedassets.
No graph6860endpoint recapture, score/history run or production change.
