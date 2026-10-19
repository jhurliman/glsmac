# Native owner0 display identity source map

RE-SMAC-NATIVE-DISPLAY-IDENTITY-001,2026-10-18. Pinned SMAC SHA256
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.

45f550 asset initializer loads6a81c8 PALETTE.PCX withpalette0/offset10/count236
at45f6b9..cc.45f6d1 EDI580,ESI0 then5fb010 samplesy23→6aeeb4,31→6aeed4,
39→6aeef4,47→6aef14. Eachownerstep x+=8,tableoffset+=4 through8owners.
Firsttwo relevant tobadge; lattertwo notassignednewsemantics. Independent PCXRLE
decode owner0 at(580,23)/(580,31) givesraw239/247,offset10bytewrap249/1.
Existing constructor reservedentries249RGB255,0,0 and1RGB128,0,0 flag4 preserved
byPALETTE install; no need full107-asset/raster rerun.

45f3a0 loadsUNITS.PCX via6a8228; templates extractedatx1+24*m,y700/731,w23,h30,
sevenm andtwovariants. At45f4a0startsownerbank77e7a0,ownerstride14*44=616.
603cb0 copies templates;45f4c0..ce remaps260→6aeeb4[owner];45f4d3..e1 remaps
259→6aeed4[owner].603c00 masks botharguments&ff and replacespixelbytes, so
markers4/3. No fullspriteextraction/rasterexecuted innewwork.

56fd50 ownerread56fda5 fromUnit+0e,storedesp1c/EDI.57003c testsowner;0 calls
515d50(unit,currentviewer9430d4) at57004f..56; nonzeroowner5d8f50(unit,0,0).
Resultesp24;5703b6..d4 selects77e7a0+(owner*14+level*2+variant)*44, then5fd9e0.
570348..37c uses samebank forscaledbranch plussecondary6aeed4 color. Unit+26
ecologyattribution isnotthisowner. No animationphase recapture.

4c8690(output,unit,opposingcontext) calls5d8f50(unit,0,context) at4c8691..a1.
4c86cb..f7:design<64 andsignedweaponpower9b6fb0+(weapon*16)<0 picks96e3fc+level*8;
otherwise96e3f8.62e860 at4c8706 resolveslocalizedrow.59e110 parser59e7d9..fb
readsfirstcolumninto96e3f8 thennativecolumn96e3fc,sevenrowsstride8.4c8a00
wrapsbuffer9c0b88; ordinarydescriptioncaller411896push0 beforeunit and4c8a00;
4c9edb..ea similarlypasses0 to4c8690. Sourceview/textcontextdistinction retained.

freeze_native_display_identity.py onlyreadsPE/asset/sourcecases, decodesPCXRLE,
verifiesstaticinstructioncheckpointsandreservedpalettepriorreceipt; nonewnative
execution. Existingcompletebirthfixture reusedlabels/lifecyclecontract. Newbundle
tmp/reference/native-display-identity-evidence-20261018. Olderfrozenreceiptsremain
unchanged; noassetpayload inpubliccases.
