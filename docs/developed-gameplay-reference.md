# Controlled developed gameplay comparison scene

The opt-in native fixture `Export developed gameplay reference campaign` reads a
user-owned GLSMAC campaign, restores its actual settings, roster, definitions and
terrain, then writes a separate campaign through the ordinary save path. It is a
controlled equivalent scene for the gameplay-map/console target, not an import of
the supplied GOG screenshot or proof of the gameplay which would earn this state.
No original asset or campaign bytes belong in Git.

The source Scout is selected by owned conventional design plan3, rather than by
the lowest conventional Unit ID. A preceding Colony Pod, Former or foreign Scout
cannot replace it. The asset-free regression exercises those three preceding
records, unchanged saved-world bytes during selection, and an absent owned Scout.

Supply an idle local SMAC campaign with an owned canonical Base, ordinary Scout,
complete worker knowledge and the default population presentation catalog. Input
and output are explicit environment paths; output must not already exist. Run:

```
GLSMAC_REFERENCE_INPUT=/external/owned.glsmac \
GLSMAC_REFERENCE_OUTPUT=/external/developed.glsmac \
./build/bin/GLSMAC --gse-tests --quiet \
  --gse-tests-native-filter '^Export developed gameplay reference campaign$'
```

Without both environment variables this opt-in export is not exercised. A missing
single variable fails. Normal CI requires no proprietary game files.

The recipe replaces entities in the isolated fixture only: three nearby land
sites retain the source terrain; Bases named New Chicago, New Austin and Hilton
Head contain9/8/10 Doctor specialists and copied production state; three actual
Scouts retain the source design and owner, clear damage/spent movement/home and
hold indefinitely on separate adjacent land tiles. Adjacent land receives roads and alternating forest or
farm/solar improvements. Population is explicit fixture setup, not simulated
growth. This is not a balanced economy: the specialist-only Bases can starve if
ordinary turns advance. All-map knowledge, memory, occupancy and native territory
claims are republished from the resulting actual entities; production guards are
not changed or bypassed by a shipped game command.

Load the resulting file through the real campaign menu. Inspect population boxes,
H overlays/health, selected-object panels, stack switching and Base navigation.
Retain original input hash, output hash, build, viewport, camera/zoom and animation
phase with each external capture. Reuse the exact exported file for repeatability;
map render RNG constructor/restore behavior has its separate regression.

The reference layout/zoom and original lossless framebuffer remain independent
acceptance dependencies. This fixture supplies repeatable populated model state;
it does not prove original projection, climate, lighting, models, fonts or UI.

After the selection correction, the actual owned-input export and mixed-unit
regression pass with allocation tracking. The new exported world and roster are
byte-identical to the prior developed scene; only the inherited settings format
changes from the intervening optional faction-summary catalog update. Existing
scene captures therefore remain valid for this fixture-only correction.
