# Native ordinary land material plan

`game::frontend::smac_land_material::Select` produces an owned ordered plan
from admitted named tile fields, a caller-supplied neighbor lookup and an
explicit original variation seed. It matches all 39 native selector contexts
and 193 ordered calls in the [original handoff](behavior/ordinary-land-texture-palette.md).
It is not yet a live map painter: geometry, lighting and loaded indexed atlas
sources must be supplied, and keyed polygon raster coverage remains separate.

The plan preserves the 256-entry native connectivity lookup, moisture UV
rotation, Farm replacement, Base/Rock interleaving, Forest diagonal mask and
Fungus connectivity/UV rotation. Triangle fan indices retain C-W-N, C-N-E,
C-E-S, C-S-W order. Quads retain W/N/E/S order, including the non-square
Fungus UV corner (55,1). Roles distinguish opaque shaded, keyed shaded and
keyed unlit passes; flags retain the original wrapper/full-helper distinction.
No coordinate wrapping or observer fallback occurs inside the selector. The
lookup owns those policies and absent/clipped neighbors contribute no mask bit.

The original local variation RNG uses the native tile-record address's low
16 bits. Its allocation-base stability is unresolved. The API requires that
compatibility input, with no default coordinate hash or consumption of another
RNG stream. It rejects absent/sea centers, invalid retained moisture/rock/band
codes and unsafe coordinate inputs. Other modes, landmarks and sea materials
are outside the measured land profile.

Palette helpers populate active entries 10..245 from the common PALETTE.PCX
at Gamma Correction100, while preserving caller-supplied reserved/system
entries. Loading rebases source indices by +10 with byte wrap. The keyed
shader tests loaded key9 before adding shade, and palette resolution follows
the final wrapped index. These helpers do not claim keyed polygon coverage.

Validation uses literal native ordered calls, all four Fungus rotations,
seam/edge/pole contexts, native palette samples, signed index wrapping and
absent-center rejection. The production C++ common-palette output from the
owned PALETTE.PCX matches all 236 native common colors, SHA256
`663f9ba0e0cf13fae99d4aaccb7e0632f79c4344f7067189ad9116b07d58df7c`.
No original asset pixel bytes are bundled. Runtime integration must preserve
admission, layer order and invalidation rather than treating this standalone
plan as whole-map RGB parity.
