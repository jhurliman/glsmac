# Live SMAC terrain source palette

The SMAC map constructor loads TEXTURE.PCX with the common PALETTE.PCX at
Gamma100. The loader maps each source index through its original +10 byte
rebase before resolving RGB, including the measured software startup reserved
entries. Source255/loaded9 is the transparency key; matching RGB values at
other source indices remain opaque. Full and cropped texture caches distinguish
this profile from ordinary embedded-palette and opaque-menu loads.

The original [land material palette receipt](behavior/ordinary-land-texture-palette.md)
and [software reserved-color receipt](behavior/scout-effect-reserved-rgb.md)
agree on all256 RGB values. The palette metadata distinguishes1024 RGB+flags
bytes from768 RGB bytes; their hashes must not be compared interchangeably.
The production SDL palette conversion matches the native768-byte RGB hash
`13c35381385725ae2858ad34c26ed918d5e62a493a0363b21126ec20e5c22e8c`
and common236-color hash
`663f9ba0e0cf13fae99d4aaccb7e0632f79c4344f7067189ad9116b07d58df7c`.

The installed embedded atlas palette differs at129 of the first236 entries,
but the17 recorded land-selector crops use matching common colors. For those
crops the visible color change is the native235 brightness cap at source92.
This fixes the source conversion boundary without claiming a large visual
change or original raster/shading parity in the current GPU painter.

The live caller selects this measured software profile only for SMAC.
SMAX and generic texture loads retain their existing profile. Saved RGBA terrain
atlases retain their serialized pixels; the new profile applies when terrain is
generated or repainted. Complete indexed terrain composition and rebuilding
presentation caches from retained state remain separate integration work.

Validation covers the actual SDL full/cropped loader, all256 source indices,
padded two-row inputs, duplicate embedded colors, a non-key common color equal
to the former RGB transparency color, cache separation and incompatible flags.
Existing menu-palette, native-badge, material, finalizer and Former repaint
groups pass with memory tracking. Focused ASan/UBSan checks the SDL conversion
against the owned common palette; targeted Map.cpp/SDL2.cpp analysis is clean.
