# Native-lifeform console overlays

Native-record Units now use the existing original owner0 UNITS.PCX flag bank
in selected and roster cards. The separate overlay remains23×30 pixels at
selected clip offset(0,0), roster offset(2,1); health occupies2×21 pixels at
overlay offset(6,6). Full0..1 UVs and nearest sampling preserve opaque black
and the original key mask. Static native body rendering remains the existing
fallback and is not claimed to match original body pixels or dimensions.

Source authority is [native-badge-raster](behavior/native-badge-raster.md),
[unit-card layout](behavior/lower-console-unit-card.md), and the existing
health-bar contract. No human palette slot is assigned to owner0. Composition
copies the retained owner0 texture instead of decoding/recoloring it again.
Native spent movement never selects variant1. Health/morale updates retire the
card cache while DOM actors independently retain their displayed texture.
Legacy Units without native-record authority or a positive maximum HP retain
the previous fallback. Conventional card/map placement is unchanged.

The actual frontend fixture covers a static non-CVR native Unit, both card
clips, exact overlay mesh endpoints/UVs, source immutability, cache reuse,
spent policy, and the native yellow14-pixel health fill. Its body is synthetic;
it is a UI caller control, not original native body evidence.

An optional runtime fixture exporter restores a supplied owned campaign through
the existing candidate-State/world path, then commits one supplied native birth
plan through the real factory, placement, sight, counters and campaign writer.
It never patches save/knowledge bytes. This is an explicitly controlled UI
fixture, not a naturally played bloom or original-game screenshot. Supply a new
output path; existing files are refused:

```
GLSMAC_NATIVE_CARD_INPUT=/path/developed-v3.glsmac
GLSMAC_NATIVE_CARD_OUTPUT=/path/native-card.glsmac
GLSMAC --gse-tests --gse-tests-native-filter '^Export controlled native card'
```

Set both environment variables for the invocation. Without them, the optional
export group performs no export. Owned campaign/art remain outside Git.
