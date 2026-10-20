# Native exploration redraw history

A retained horizontal pan previously checked camera, zoom and relief, but not
the observer knowledge mask or survey lighting. If the caller changed a mask in
place, the dirty strip could update while the interior retained revealed or
unknown pixels from the previous presentation.

`ViewGeometry` now owns the last successfully prepared knowledge mask and
survey-light identity. Exploration changes, reveal/observed transitions and
survey presentation changes prepare a complete view. Equal masks at different
addresses keep the measured horizontal-pan path. Skipped paint and invalid
knowledge dimensions do not publish new presentation history. The frontend
passes retained bitmap history only when the returned plan actually has dirty
regions, rather than assuming its incremental request was accepted.

Six raster regression cases compare the changed presentation with a complete
fresh render, including unknown-to-known and known-to-unknown interior tiles,
unknown-to-survey, survey-light changes and transitions to/from revealed
inspection. Tests also check equal-mask pan reuse, skipped paint, rejected
dimensions and rejection of stale bitmap input after a full redraw.

Validation: cached incremental build; 24 selected native groups pass normally
and with focused ASan/UBSan instrumentation on the two renderer/frontend
consumers, texture loader and terrain test unit. Other dependencies are normal;
leak detection is disabled. Correctness analysis passes the three production
consumers with no failures. No platform matrix repeated.

These are synthetic renderer/caller checks. They do not establish an original
framebuffer comparison or implement ordinary gameplay's unexplored black grid.
That still requires connecting the authoritative observer snapshot and native
view policy, interactive camera, picking and all terrain/actor layers. The
current opt-in inspector remains explicitly revealed.
