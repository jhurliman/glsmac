# Native ordinary terrain lighting

`game::backend::map::smac_terrain::OrdinaryLighting` computes the signed
C/W/N/E/S palette-index light inputs from retained altitude bands. These are
the native discrete incident-face rules, independent of relief pixel heights
and geometric mesh normals. They are not RGB tint multipliers. The indexed
painter must consume them before final palette lookup; the existing GPU
terrain painter is not replaced by this producer alone.

The 196-entry face table is generated with native quarter-turn orientation,
integer square root and signed truncating division. Boundary corners use the
native slope code; ordinary corners retain the four-contributor modulo16
code. The center floors the four-face mean. Each outer vertex floors its
eight incident face contributions. Negative means use explicit floor division,
not C++ truncation or a host-dependent signed shift. The shared corner offsets
now live in a small common header used by both heights and lighting.

The lookup callback owns topology and admission. Missing/sea face contributors
give zero light and boundary slope behavior. Missing/sea centers, invalid
bands, unsafe coordinate arithmetic and slope selectors outside the measured
[-3,3] face-table range return absence. Landmark deformation and larger slope
jumps remain explicit open profiles; they are not silently clamped or assigned
invented light values. No RNG or retained world state is modified.

The original [terrain cases](behavior/ordinary-terrain-projection-cases.json)
now include complete radius4 band stencils for all 16 native land contexts,
plus the 196 original-checked table entries. The old radius2 excerpts were
sufficient for heights but not for all incident-face inputs. Stencils reuse
the frozen synthetic setup and SHA-verified installed-map fields; all prior
numeric inputs and expected outputs are unchanged. No native geometry rerun
or original asset payload is bundled.

Validation compares every face-table entry and every vertex of all 16 native
land contexts, including original Planet samples, round seams, flat-map and
polar boundaries, coastlines and low relief. Existing 18 height contexts,
actual camera projection and actual finalizer consumers remain covered.
The new producer is also analyzed with its affected height/finalizer consumer.
