# Dialog GDI glyph controls

These are **current Windows GDI offscreen controls**, not execution of an original
modal dialog or an original desktop screenshot. The source-derived requests and
stock registration of only regular `arialn.ttf` are established by the earlier
`generic-dialog-fonts-and-frame.md` contract. No font payload or glyph image is
published here; owned-resource masks remain under private `tmp/reference`.

Registering only bundled regular `arialn.ttf`, requesting Arial Narrow at height
−16, weight700 selects that same regular font data (SHA256
`e17b243cbf2e194890c561dbb63e515dc34e941ee1a7923748b336199b812f52`).
Weight700+italic also selects that regular resource. The observed masks establish
GDI synthesis: weight700 differs from the weight400 regular control for94/95 ASCII
characters and increases total ink pixels from1780 to3178. Italic weight700 has3306
ink pixels. The weight400 control is diagnostic; the native >12 font-bank columns
request effective weight700 even for the nominal plain column.

In separate processes, registering bundled `arialnb.ttf` and `arialnbi.ttf` instead
selects their own font data. Their bold masks differ from the regular-resource GDI
synthetic bold for92/95 ASCII characters; bold-italic differs for94/95. The Arial14
bold caption control matches across both registration conditions for95/95 glyphs.
These are binary equality counts, not perceptual similarity measurements.

The private raw format is one byte per pixel, **0=background, 1=ink**. Each file is
97280 bytes:95 cells, ASCII32..126 in ascending order; each cell is32×32, top-down,
row-major, stride32. Character origin is x4, baseline y24 inside its cell. There is
no per-glyph cropping or placement normalization. The companion PNG is a16-column
grid with the same32×32 cells; its unused final cell is background.

`regular.json` contains four profiles: Narrow16/700, Narrow16/700 italic,
Arial14/700, and Narrow16/400. `styled.json` contains the first three requests with
the separate styled-resource registrations. Each profile records all95
`ascii_advances`, signed A/B/C bearings in `ascii_abc`, the complete60-byte LOGFONT
in `font_metrics.LOGFONT_hex`, and56-byte TEXTMETRICA in
`font_metrics.metrics_hex`, plus selected-font-data, raw-file, per-cell and PNG
hashes. Advance equals A+B+C for every control. All requests use default quality0,
out precision7, ANSI charset0; other LOGFONT fields are recorded exactly.

Rendering uses `TextOutA`, transparent background, baseline alignment and an
offscreen top-down8-bit DIB with a binary black/white palette. No monitor DC,
foreground window or pointer interaction is used. Only the specified resources
are registered process-privately and removed afterward. These binary indexed-DIB
masks should be compared with the port's hinted rasterizer and synthetic transforms
before choosing a replacement font. They do not establish identical results for
another Windows/font version, antialiased32-bit rendering, extended codepages,
kerning, or arbitrary sizes.

Private root: `tmp/reference/dialog-glyph-masks-development-20261019/`.
The frozen evidence bundle copies these files under the same repository-relative
paths. Public numeric metadata and hashes are in
`dialog-gdi-synthetic-glyphs-cases.json`.
