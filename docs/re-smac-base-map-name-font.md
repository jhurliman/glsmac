# Private Base map-name font and advance receipt

Pinned terran.exe SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Public [contract](behavior/base-map-name-font.md), [cases](behavior/base-map-name-font-cases.json). Frozen bundle `tmp/reference/base-map-name-font-evidence-20261019`. Prior terrain bundle remains immutable and complete; no terrain probe is repeated for this text task.

## Actual caller and execution boundary

Base name is in map-view routine4743e0, not5719b0's population badge, lower-console4c9880 title, or474f80's order labels. 4743e0 checks view name-enable flag1dd70bit800, preference959f9cbit2000 and zoom>−11. It loops Base records; ordinary ownership/knowledge admission is at4744b2..4744e1. Actual coordinate projection46f930 is called47456f. It selects view+1eec8 font at474574..47458d, copies Base+19 name at474592..4745ce, optionally appends a status suffix via another branch, and sets owner text/shadow metadata at474617..474640.

4746c0..474714 calls centered text5f6050(name, anchorX+view+1ddbc, anchorY+SAR(7*view+1ddc8,1), length). Probe stops **at474719**, immediately after this actual draw returns, before optional status/production/debug labels. Seven supplied contexts reach that exact boundary. It is not called a full4743e0 return or a live game draw.

`probe_base_name_font.py` composes only the writer-free prefix of `probe_map_overlay_text_api.py` before `ptr=get(u,bitmap+0x54)`. It retains the full native text stack, rather than the old5f4b00 observation sink. That prefix composes the established original constructors, gamma100 palette preparation, installed palette/UNITS metadata and current Windows font services. No prior frozen image/overlay case loop runs or writes.

Supplied view0x22000 backing uses generic-window vtable offset0; actual bitmap5f0010/5f0470 creates embedded map bitmap atview+444,1024×511. It establishes native width/signed-height fields. Camera8,8, zoom0, no scale override, name view flag800, name preference2000, Base count1 and initialized game-admission field944608=1 are supplied. Existing declared80×40 map and zero-height projection backing are reused. Base0 owner/name/coordinate/population are explicit; focus equals owner, secondary flags0, no status suffix. This is not native founding, Planet generation or organic viewport construction.

## Original font request

Complete46f3b0 view initializer executes with cached zoom poisonedFFFFFFFF to enter font creation. 46f863..46f89d computes `min(12,trunc(5*tilePixelHeight/16))`, flags0, face[6affac]. At zoom0 tile pixel height48 gives15, capped12. Font objectview+1eec8 therefore receives Arial−12, weight0. Separate view fonts+1ef20 italic and+1eef4 bold are created but are not selected for the Base name. Height family selection uses original630450→CreateFontIndirectA; face/global constructors selectArial. Arial Narrow's installed font resource does not make this caller an Arial Narrow font.

Accepted LOGFONT fields: height−12, width/escapement/orientation/weight0; italic/underline/strikeout/charset0; output precision7, clip precision0, quality0, pitch/family0. Full raw60-byte buffer is retained; trailing bytes after face NUL are uninitialized stack data, not face content. Current Windows TEXTMETRIC resolves weight400,height15,ascent12,descent3,internal3,external0. Native font+10=9, used by5f4b00 to convert logical y to GDI baseline. Font line-advance override remains−1; this single-line name is not the13-pixel-spaced lower-console panel.

## Width, advance and kerning evidence

5f6050 measures through5f55c0, subtracts SAR(width,1) from the center x, then calls5f58e0. Native630870 selects the font into global measurement DC9c15ec and calls GetTextExtentPoint32A. Complete5f4b00 reaches TextOutA for one complete shadow string and one complete foreground string, then measures its returned advance. No glyph-loop result or explicit dx/kerning array is supplied by the original name caller.

The host width service is wrapped only to **observe** requests. All six strings are measured as whole strings by the original path. Additional GetTextExtentExPointA cumulative advances and GetCharABCWidthsA are queried on a current Windows8-bit indexed DC with the same HFONT. All full-string widths equal cumulative last values and sums of the independently queried character advances. i=(A1,B1,C1), W=(0,11,0), j=(−1,3,1), f=(0,4,−1). Each reported advance is A+B+C. These are supplementary GDI API metrics, distinct from actual monochrome raster ink bounds.

Widths are62,74,65,42,12,52 for New Austin,New Chicago,Hilton Head,iiiWWW,jifj,AV To Wa. iiiWWW cumulative positions3,6,9,20,31,42; jifj3,6,9,12. GetKerningPairsA advertises AV−1/To−1/Wa0 on this pinned font, but native measurement and GDI cumulative positions retain AV14 and To14. Hence no pair adjustment is applied in this checked plain-name path. This is not a Unicode/shaping or every-font guarantee.

## Colors, baselines and raster

474620..474640 reads owner text foreground6aeef4 and shadow6aef14 (four-byte stride), calls5f3ab0(fg,shadow,1,1). Original metadata loader samples palette.pcx x580+8*owner, y39 and47. All eight metadata pairs are retained, though actual map-name paints cover only University and Spartan. Normal University155/83 has RGB216,224,235 /27,33,31; Spartan75/65 is137,166,166 /0,0,0. Debug branch474645..4746ba can substitute white/black under special secondary flags; all accepted inputs exclude it.

Complete5f4b00 draws shadow first at(+1,+1), then foreground, using SetTextAlign24/SetBkMode1 and DIBINDEX colors0x10ff0000|index. Captures record both actual calls, font, color, coordinates and selected clip. Whole-bitmap clipping selects no custom region in these cases. Replay also enforces the native1024×511 surface bounds inside a1024×768 diagnostic canvas. Flat anchor projection gives label baseline27 pixels below flat tile center: box anchor−24, caller+42, primitive+9. Width65 centers bysubtract32.

Current Windows offscreen indexed GDI realizes the glyph pixels. Diagnostic background80 is separate from foreground/shadow. Seven per-case PNGs and one composite contain only those three exact indices, with no antialiasing shades. PNGs and Arial file hash are pinned. This is not a live original framebuffer, historical Arial raster, terrain composition, or a claim about the port's current font file. Source/font metrics and actual pixel receipts are kept distinct.

`check_base_name_font.py` validates all seven contexts,14 actual TextOutA calls, exact logical font/metric fields, literal widths and bearings, kerning discriminator, centering/baselines, palette values/shadow offset/order and8 PNG hashes. No production file/build, broad RE goal, Hold or terrain rule changed. The completed terrain contract requires no further work under this text request; its remaining dependencies stay explicitly listed in that handoff.
