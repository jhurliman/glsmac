# Private ordinary Hold faction-color supplement

Pinned executable SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Public [contract](behavior/ordinary-hold-glyph-colors.md) and [cases](behavior/ordinary-hold-glyph-colors-cases.json). Frozen bundle `tmp/reference/ordinary-hold-glyph-colors-evidence-20261019`.

`probe_hold_glyph_factions.py` composes `probe_map_overlay_text_api.py` only before `ptr=get(u,bitmap+0x54)`. That composes the existing overlay fixture only before its case loop, removes the5f4b00 text sink, and retains the established original font/text path to captured SetTextColor/TextOutA. It does not rerun old case loops, image writers or overwrite prior artifacts. Reused services sink body/model/admission/morale inputs as documented in the overlay fixture; no claim of full world visibility or original body raster.

Actual installed alpha.txt #FACTIONS is parsed to obtain `GAIANS,HIVE,UNIV,MORGAN,SPARTANS,BELIEVE,PEACE`, then those identities are explicitly supplied at95a4e4+owner*59c. This verifies the renderer's retained identity consumer, not a new faction-file loader path. Owner is Unit+14; order+17=2; stateDWORD+4=0; damage+16=0; spent+40=0; all32 calls are complete56fd50(bitmap,Unit0,100,100,zoom0,flags0) returns. Full original5f4b00 reaches actual TextOutA with captured Arial Bold−12 font and indexed foreground; glyph inference from screenshots is unnecessary.

## Color source

570669..570690 selects the native badge font through56fb30 and bitmap font setter5f3a70. 570695..5706dd compares the owner's retained filename byte-for-byte against literal6a82dc, which is `SPARTANS`. It is a case-sensitive C-string comparison. 5706dd..5706ef converts equal→index44, unequal→index0 using neg/sbb, low-byte ANDd4 and add2c, then calls5f3ab0(foreground,FFFFFFFF,1,1). There is no shadow. Actual palette959aa8 contains index44 RGB226,203,176; index0 black. Supplied identity swaps separate the filename rule from canonical faction slot5. Diagnostic lowercase fails the exact comparison.

Status foreground overrides are separate: 5709b5 tests state200 and can select faction secondary color; 570a04 tests8000000 and selects250 or255; 57096b tests40000 and can select248/251 on its admitted non-owning/nonhuman path. These are excluded by state0 in all accepted contexts. Do not interpret their inventory as a new status contract.

## Viewer mask source

5708fd loads viewer9430d4, owner from the draw frame, and low byte of959ff0 human-player mask. If owner==viewer and that human bit is set, it jumps directly to57099b. Otherwise570927..57093d checks scenario/editor bit80 of959fc8: when clear and the viewer's human bit is set,57093f stores `-` and jumps5709ad. Human-owner bypass and foreign-human replacement both occur after status glyph selection. In the no-status scope neither changes the selected color.

Editor bit80 bypasses forced-dash selection. When the viewer bit is clear, it also bypasses that replacement. Additional flags40 combined with1000 can substitute Unit+43 at57094c..570960; these debug branches are excluded by flags0/80 here. No treaty field is consulted in the mask block. This is an order-disclosure mask within an admitted draw, not a complete ownership/fog/treaty visibility algorithm.

57099b escapes bracket glyphs;570a2f..570a54 invokes centered text5f5f30. Full5f4b00 outputs H at111,116 or dash113,116 (baseline), fontArial Bold700−12, SetTextAlign24, SetBkMode1. Native centering accounts for the differing measured widths. The accepted32 contexts all assert glyph from viewer/owner/human-mask/editor conditions, index44 iff exactSPARTANS, font/weight/height, and GDI mode. No new PNG generation was needed to identify SetTextColor arguments.

Bundle pins original source listing56fd50 and font/text listings, all writer-free-prefix dependencies, installed alpha/palette/units assets hashes and the existing overlay manifest hash. Source-only status/debug paths remain out of scope. No Hold-rule reinvestigation, production edit, full game screenshot, AI policy, SMAX/PRACX or broad-goal continuation.
