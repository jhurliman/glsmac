# SMAC population badge rendering

The [original overlay receipt](behavior/map-unit-and-population-overlays.md)
specifies Arial Bold with logical pixel height16, a one-pixel frame, text at
left+3/baseline+14, and widths measured from `8`/`88` plus5. With the measured
nine-pixel advance, populations9/10 have14x16 and23x16 frames. There is no text
shadow. The new stock SMAC path uses monochrome Arial Bold16 and these native
texture dimensions, rather than a padded48-pixel Arial Narrow texture.

Occupied badges use the original stock faction primary palette entry, black
text, and black frame; Spartans use their measured secondary frame and text44.
The existing unoccupied style uses a transparent interior and faction text
entry. Colors come from the installed software palette through its existing
bounded decoder. Stock identity selects colors independently of transport slot.
The resolved Base artwork's primary/secondary/text metadata must match the stock
indices, and its imported text/shadow/border colors must match the configured
Faction. Replacing a PCX under the same ID or overriding colors in script falls
back to the configured renderer. Common software-palette channels are capped235
at gamma100; system-reserved colors such as University white remain exact.
The sprite uses the Unit flag's pixel scale and keeps its first corner fixed
as the population becomes two digits. Existing Base refresh and sprite retirement
remain the callers. Other rulesets and custom faction identities retain their
existing renderer.

The native tests pin resolved stock/mod/script color admission and the software
common-color cap, plus measured widths, baseline, digit advances, border and
transparent/filled interiors independently of host font availability. The cached
incremental build passes this test, the health-bar groups and frontend Base
ownership/retirement navigation. Five affected production translation units pass
targeted correctness analysis.
Focused ASan/UBSan instruments five population/Base/test translation units and
passes the raster and Base ownership/retirement groups; other objects reuse the
normal build and leak detection is disabled.

A real software-rendered session loads the retained research question through
Title → Load Game, accepts empty priorities and displays Gaia's occupied
population1 badge. The log confirms actual Arial Bold16 loading and14x16 texture
creation. Normal Menu → Game → Quit exits0 with no tracked OpenGL leaks. Private
capture at d290eead (before the metadata/color-cap review follow-up):
`.codex-validation/population-live-capture/accepted.png`; the earlier
automated attempt timed out during loading and is not a successful capture.
Audio was disabled; heap allocations and hardware frame rate were not checked.

This is not full reference parity. The developed reference scene, population10
live capture, original font raster comparison, absolute Base anchor, all zoom
policies, three-digit overflow and foreign/knowledge occupancy styles still need
validation. The frontend's existing occupancy predicate is unchanged. SMAX
presentation needs its own measured admission before sharing this path.

Comparing the top terrain crop of this capture with the console agent's capture
from the exact same saved bytes still changes169455/183896 pixels. Searching
translations of±3 pixels gives best mean channel difference12.34/255 at(0,0),
so a one-pixel console-height change does not explain the variation. This is
diagnostic evidence, not a controlled same-binary repeatability test or an
original-game diff. Resolve presentation variation before global pixel gating.
