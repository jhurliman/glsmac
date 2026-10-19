# Controlled developed gameplay comparison scene

The opt-in native fixture `Export developed gameplay reference campaign` reads a
user-owned GLSMAC campaign, restores its actual settings, roster, definitions and
terrain, then writes a separate campaign through the ordinary save path. It is a
controlled equivalent scene for the gameplay-map/console target, not an import of
the supplied GOG screenshot or proof of the gameplay which would earn this state.
No original asset or campaign bytes belong in Git.

The source Scout is selected by owned conventional design plan3 and resolved infantry Scout presentation, rather than by
the lowest conventional Unit ID. A preceding Colony Pod, Former or foreign Scout
cannot replace it. A lower-ID owned Spartan Rover also cannot replace it. The asset-free regression exercises both roster contexts, unchanged saved-world bytes during selection, and an absent owned Scout.

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

Further review controls: a locally owned, lower-ID selected Spartan Rover remains in the world when the ordinary Scout is removed; selection returns absence rather than substituting it. Retired Base history is retained, while the opt-in construction restores each declared public scene label after ordinary Spawn registration/territory hooks. The declared name is already registered either by the retired Base or the new spawn. No production name policy or mutation guard changes.

The focused mixed-roster and retired-name regressions pass with allocation tracking. Actual exports from both the original University input and a previously developed three-Base input pass with allocation tracking. Exporting the original input remains byte-identical to developed V4 (SHA256 `4f5bf3435f937b51695b8728655476ac0b82388e5a48d612a8105111c69069da`), so earlier scene images are reused without another graphical/platform run. These review fixes change test-fixture code only; prior sanitizer receipts cover the unchanged ownership path, not a fresh sanitizer run of the new cases.

Site-cache follow-up: before suspending knowledge for isolated entity reconstruction, stage native `RemoveBase` writes for all old Base centers, with no surviving centers because the entire set is replaced. Publish that detached cache and retain unrelated site scores/history; missing legacy caches stay missing. This avoids ghost exclusion radii without weakening the generic live-knowledge mutation guard. The cache-present regression starts with two separated Base marks and an unrelated score9, proves all old exclusions are gone and the score/source cache are preserved, then marks a new Base. Owned original-input and developed-input exports also pass with allocation tracking. The original-input output remains byte-identical to V4; this legacy input does not establish the cache-present path, which is covered by the explicit native cache regression.
