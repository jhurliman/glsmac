# Owned observer view preparation

`ViewContext` separates current UI presentation from backend terrain snapshots:
view index/active state, full draw flags, zoom, camera tile, raw preferences and
relief. It is created once for a new frontend view, retained across appearance
updates and retired with that view. Observer identity stays in the owned world;
it is not inferred from view index or fixed to the reference faction 3.

The ordinary primary flag word is `0x60010ee3`. The captured missing-profile
preference fallback is `0xbbe1ddb6 / 0x003a7168` for non-laptop difficulty below 2;
this is not an INI/profile loader or a new campaign view codec. The existing
revealed inspector explicitly supplies its separate `0x10621 / zero-preferences`
profile. Numeric source is the frozen ordinary-view-policy handoff, manifest
`c3363521e8039c43263dba8df5c52cbdf998c003845178ae39170018d46b1ce5`;
all 11 listed files were verified before use.

`Session::Prepare` now consumes the owned world's `KnowledgeAt` contract.
Explored/local-revealed cells become known; unvisited cells are unknown or
surveyed according to the retained game rule. Survey uses initialized palette
light 2, not RGB gamma or ordinary known-terrain face lighting. Missing observer,
exploration or required survey authority produces no plan and retires retained
bitmap history. Explicit revealed inspection is a separate enum choice.

The session owns its knowledge history, keeps the measured pan path for unchanged
presentation and requests full redraw for observer or preference changes.
Appearance invalidation drops bitmap/layout history without resetting current
UI camera or policy. Unsupported geometry cannot enter the fixed-size session.
Bitmap/layout and observer identity publish together after rendering succeeds;
an abandoned preparation forces the next preparation to redraw fully. Tests
also switch observers with equal masks, so observer invalidation is not merely
an incidental result of different exploration bits.
The frontend inspector now uses this preparation path, and the original-frame
probe uses its ordinary observer mode instead of building a mask independently.

Validation: cached incremental build; 33 selected native groups, including an
actual backend MT saved-exploration snapshot through view preparation. That test
also checks simulation RNG, saved knowledge and backend geometry remain unchanged.
Four production consumers pass correctness analysis. All 13 complete original
known-grid frames still match (6,802,432 indexed pixels), including the same
ASan/UBSan standalone renderer comparisons across 12 units, leak checks disabled.

A hidden Linux SDL/OpenGL run loaded the existing Morgan 2105 campaign
(SHA256 `aed85217e7684412e0de762d4ccf23df5892f20fc0355a8b2fa15ea55580ae92`),
rendered with the explicit revealed inspector, confirmed the unmoved-unit prompt
and advanced to 2106. Normal Quit/confirmation exited 0 with no tracked OpenGL
leaks. Allocation debugging and audio were disabled; this is not an audio,
performance, native-Mac or original-game framebuffer comparison.
That live run preceded the final uncommitted-preparation guard. The final guard
and commit path were checked with the native groups and instrumented original
frame comparisons; the unchanged successful-path visual run was reused.

Ordinary main-map presentation is still gated. Terrain picking/discrete zoom,
all legacy terrain/prop suppression, actor rendering and profile/save ownership
still need connection before that gate can be removed. Current context is not
yet restored from campaign saves. This PR does not close normal-game fog,
color/coast/relief acceptance or full actor pixel matching.

![Actual refactored inspector after advancing to 2106](https://raw.githubusercontent.com/jhurliman/glsmac/50c6219c961816b114aa77ec48b8b663cb477ad0/native-observer-live/advanced.png)
