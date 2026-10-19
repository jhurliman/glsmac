# Faction console text rendering

The stock-SMAC original-x86 contract measured in `lower-console-faction-text`
(2026-10-19) supplies the font request, colors, baseline, row advance and clips.
Its public cases SHA256 is
`e162c174fde8751345110ddf2801224642fc7fd719fb3d368c1cba3f7d7aa34b`.
The original executable, fonts and controlled offscreen GDI rasters remain
external to the repository.

At 1024×768 the console begins at screen y511. The central frame is
`[262,66,763,172)` in console coordinates. The two text clips are
`[269,73,495,165)` and `[498,73,724,165)`, with exclusive right/bottom edges.
Origins are five pixels inside each clip; the first baseline is clip-top+14
(logical row y+9), and subsequent baselines advance13 pixels. Text does not
wrap, shrink or gain an ellipsis. DOM text actors use their lower geometry edge
as the baseline, so positioning subtracts each string's measured height.

The font is regular Arial at12 pixels, corresponding to native logical height−12,
with solid monochrome rasterization matching the accepted indexed-DIB receipts.
The native DEFAULT_QUALITY request itself does not mandate monochrome on every
host. Arial comes from installed game data or system fonts, with the existing
visible fallback warning if unavailable; no font is bundled. Label color is
`#749c38`; inline values use `#b8c060`. Value origins use measured prefix advance,
preserving the native colon and space. Faction Dominance uses the same regular
font, label color and first-row baseline.

Faction/leader identity remains retained source metadata. Integrity, Might,
Council Votes and historical curves have no admitted live model projection yet;
the panel explicitly displays unavailable values. This change does not invent
ratings, zero votes or history. Original partial-reveal timing and alternate-color
phase scheduling are not measured; the current page shows the fully revealed
normal phase. The source-only800-pixel font branch and special-game dominance
caption are outside this1024-pixel parity target.

Current Arial/FreeType and Windows GDI may differ in hinting and glyph outlines.
Matching the measured layout/font request is not a claim of historical font
raster parity or original full-game framebuffer parity. A runtime comparison
must distinguish supplied original test strings from actual campaign identity.
