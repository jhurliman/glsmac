# Live faction selection preview

The faction selector imports each catalog definition's descriptor, BLURB and
ordered DATALINKS1/DATALINKS2 text. Highlight changes update the leader portrait
and quotation. Faction Details switches to the background, agenda and rules
view. The preview disappears during name entry and reappears for the retained
selection on Back; Cancel removes it with the faction dialog.

Text remains in the user's installed TXT files. Carets delimit paragraphs;
physical continuation lines join before wrapping to measured text width.
Curly emphasis delimiters are removed for display; styled spans remain open.
Portraits use the original131x157 rectangle, the common active palette at
Gamma100 and the loaded index9 key. The explicit indexed import bypasses
legacy faction RGB shadow/transparent-color filters. This reuses the existing
software palette conversion; it adds no scanline mask, tint or extra gamma.

The original behavior is documented in
[setup continuation](behavior/setup-dialog-continuation.md) and the newly
verified [portrait contract](behavior/setup-portrait-render.md). The private
nine-file receipt was hash-checked before importing the public facts; its
manifest SHA256 is
`4d3d2366c93286cbef777a4cf3ef89f4fc8a9ce434039cff61a22009d46c938c`.
Those original findings are static; no new original-executable UI run is claimed.

## Validation

Cached incremental build; the native preview-parser group; four GSE scripts
(preview, identity, faction selection, ruleset navigation); changed-production
analysis of FactionTXTLoader, FactionManager, Surface and TextureLoader plus
the affected SDL2 loader consumer, all with zero findings.

A real1024x768 SDL/OpenGL run exercised all seven SMAC faction previews,
Morgan's details toggle, name entry and Back, then normal title-menu Exit
(exit0, no tracked GL leaks). An earlier run exposed the legacy filter
conflict and failed; only the repaired run supplies the images below.

For each of seven captured portraits,20,563 opaque pixels match the independent
source-derived Gamma100 RGB expectation exactly, at framebuffer position60,60.
The four transparent corners are excluded because they reveal the backing UI.
This is an asset/conversion comparison, not an original-executable full-dialog
screenshot diff. Repeat with Pillow, installed reference assets, and one
`<faction-sheet-stem>.png` per profile:

```sh
python tools/check_faction_portraits.py --assets /path/to/game --captures /path/to/captures
```

![Actual Morgan quotation preview](https://raw.githubusercontent.com/jhurliman/glsmac/b3f8914f73ced8e76fbf6501cd6a4e52ab396870/faction-selection-preview/morgan-quotation.png)

![Actual Morgan faction details](https://raw.githubusercontent.com/jhurliman/glsmac/b3f8914f73ced8e76fbf6501cd6a4e52ab396870/faction-selection-preview/morgan-details.png)

## Remaining fidelity

This connects the real preview producer; it does not certify dialog pixel
parity. Fonts, text/frame spacing, original right-aligned role descriptions,
emphasis spans and the OTHER definition chooser remain unfinished. The port's
Random entry and extended catalog selection still differ from the original
loaded-slot selector. The source's preview position/width parameters inform
the current layout but are not proof of exact final native placement. SMAX
preview code uses the same catalog path; only the seven SMAC portraits were
captured and compared in this pass.
