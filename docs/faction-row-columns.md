# SMAC faction row columns

The SMAC faction picker now paints the formal name and parenthesized descriptor
as separate columns. The left origin is6; the descriptor advances end at522 in
the525-pixel row. Both labels use the original parent-relative baseline14 and
share font, color and transform changes. Row spacing remains19 pixels.

The earlier layout receipt said519 for the right edge. That prose was wrong;
its captured coordinates already used522. The supplemental
[source and metric correction](behavior/faction-descriptor-right-alignment.md)
traces the original padding2, right shrink by padding+1, and GDI extent call.
All eight measured descriptor runs have additive advances and zero overhang.
The12-file frozen supplement was verified against manifest SHA256
`5abf3c7b5024b07085dfa03a3316410f7b2d4444f865ce86758c8bdc319219c1`.

`Button.righttext` creates an embedded label only when a nonempty second column
is supplied. Clearing/replacing text reuses it; normal buttons retain their
single label. The late label receives the current styles without converting
class colors into manual overrides. Subsequent hover/selection changes therefore
reach both columns. Clicking the descriptor uses the same row selection path.

`Text.baseline` is an optional logical-pixel offset from the parent control's top;
`Button.textbaseline` forwards it to both labels. Without it, existing text uses
its previous geometry-bottom baseline. This explicit coordinate avoids different
glyph heights moving independently centered strings onto different baselines.

## Validation and comparison limits

The cached incremental build, four affected GSE scripts, three native UI groups,
and scoped analysis of the three changed production translation units and two
choice/select consumers pass. Focused ASan/UBSan instruments Button, Container,
Object, DOM Text, geometry Text and the native UI test. The native test verifies
lazy allocation, late style inheritance, selected/hover transitions, actor reuse,
the parent-relative baseline, right inset and actor removal. Other objects use
the normal build; leak detection is disabled in that focused sanitizer run.

Actual1024x768 SDL/OpenGL captures were compared directly with the original
525x130 list DIB, without local shifts or image warping. Six rows in comparable
states cover50,400 pixels and differ at7 glyph pixels: counts1,1,2,2,0,1 for
rows0,2,3,4,5,6. These correspond to the previously recorded Narrow16 synthetic
font scan-conversion residuals. The seventh original row is hovered while the
capture's row is ordinary; it is excluded from that comparison, not declared
equal. Descriptor-click selection and normal window-close cleanup also pass.

This does not establish whole-screen parity. Preview panel layout, buttons,
gender/customization continuation, edit selection/caret and the remaining font
pixels still need work. SMAX retains its existing picker path.
