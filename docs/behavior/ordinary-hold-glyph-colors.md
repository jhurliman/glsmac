# Ordinary Hold glyph: faction color and foreign masking

Stock SMAC, 2026-10-19. Supplement to [map overlays](map-unit-and-population-overlays.md), with [32 compact cases](ordinary-hold-glyph-colors-cases.json) and [private source/execution map](../re-smac-ordinary-hold-glyph-colors.md). Original Unit renderer and complete text stack execute through `TextOutA`. No production code changed.

For an ordinary no-status conventional Unit at zoom0, retained Hold displays **H** to its owning human viewer. Foreground is selected by the Unit owner's retained faction filename/identity, using an **exact, case-sensitive comparison to `SPARTANS`**:

| Stock filename | Owned Hold | Ordinary human foreign viewer | Foreground index / gamma100 RGB |
| --- | --- | --- | --- |
| GAIANS | H | - | 0 / (0,0,0) |
| HIVE | H | - | 0 / (0,0,0) |
| UNIV | H | - | 0 / (0,0,0) |
| MORGAN | H | - | 0 / (0,0,0) |
| SPARTANS | H | - | **44 / (226,203,176)**, `#e2cbb0` |
| BELIEVE | H | - | 0 / (0,0,0) |
| PEACE | H | - | 0 / (0,0,0) |

Identity-swap controls prove that `SPARTANS` in slot3 still uses44, and `UNIV` in slot5 still uses0. Diagnostic lowercase `spartans` uses0. Do not key this exception to player number, display name, badge primary color, or an ally's faction.

The ordinary foreign mask changes the **glyph to `-`**, retaining the original faction-dependent foreground. It does not render a visible H in another color or make H transparent. The native conditions, after ordinary glyph/status selection, are:

- An owning human viewer retains H.
- For a different viewer whose bit is set in the human-player mask, with scenario editor mode off, the glyph is replaced by `-`.
- With scenario editor mode on, the foreign H is retained in these controls.
- If the viewer's human-player bit is clear, the ordinary foreign mask is not applied. That diagnostic nonhuman-focus path retains H; it is not permission to expose foreign orders in a human UI.

All seven faction controls cover owned-human, foreign-human, foreign-editor and foreign-nonhuman-focus contexts. An additional owned nonhuman-focus control retains H. The foreign mask path performs no treaty check, so an ally status is not a reason to bypass this particular mask. Complete world/fog admission is outside this supplement; the Unit must already have reached overlay rendering.

Font stays **Arial Bold700, logical height−12**, transparent background, baseline alignment, no shadow. At anchor100,100 the measured native H call is x111/baseline116; the dash is x113/baseline116 because it is centered using its own width. Existing logical order box is `[110,105,121,119)`. Thus use the same box and native-style centering, not H's x offset for every replacement glyph.

All supplied Unit status bits are zero, damage and movement-spent are zero, and draw flags are zero. Automation/status foreground overrides, exceptional/native Units, blink admission, transport rules, fog, cheats/debug overrides and other zooms retain their separate contracts. This does not change [Hold gameplay behavior](human-hold-order.md).

Frozen evidence: `tmp/reference/ordinary-hold-glyph-colors-evidence-20261019`, containing32 complete renderer/text contexts, source/dependency snapshots, JSON and validation log. This task reused the prior font/raster harness without rerunning or overwriting the frozen overlay PNGs. Broad reverse engineering remains paused.
