# Pending research choice UI

The game UI reads `get_pending_smac_research()` at setup and on each
`research_choice` notification. A dedicated modal lists the supplied ordered
technology names/categories and the owning faction. Selecting a row and
confirming sends only its request ID and technology ID to `choose_research`.
It neither calculates eligibility/cost nor chooses automatically. Repeated
confirmation is suppressed until the backend replaces or clears the request.
Escape cannot dismiss it; returning to the main menu uses the existing reset.

The real GSE module passes `smac_research_choice.gls.js` on the cached executable,
covering initial and later requests, all four category labels, bonus replacement,
stale row callbacks, duplicate confirmation, authoritative clear and reset.
DOM and backend API are doubled in this script-only test. The shared
`_smac_research_choice.gls.js` helper also accepts an actual Game wrapper for the
native paused-turn integration test. No rendered UI or linked research
continuation result is claimed by this checkpoint.
