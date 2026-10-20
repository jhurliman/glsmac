# Base body and primary-name disclosure

The live frontend now applies the ordinary disclosure part of the
[original Base admission contract](behavior/live-object-name-stack-admission.md).
An owned or stored-disclosed Base name can remain visible on an unknown square.
The model and population badge additionally require explored terrain or the
actual viewer's faction reveal. Foreign faction reveal cannot disclose it, and
local reveal does not waive a foreign Base's stored discovery requirement.

`Base` retains independent body and name visibility. `Hide` clears both even
when the body was already hidden. Identity replacement, artwork selection and
projection refresh preserve a name-only Base. Population/badge resources are
still refreshed before a body is shown again. Snapshot delivery refreshes all
registered Bases; a missing roster entry removes its old presentation.

Ordinary SMAC presentation waits for owned authority. The explicit revealed
inspector retains its intentional reveal, and legacy synthetic views keep their
existing presentation. The source's other display flags/preferences, editor
contexts and zoom cutoff remain separate integration work; this change does not
claim the complete original Base view selector or original screenshot parity.
Unit/head/stack gates and main-map unknown black/grid drawing remain open.

Validation: nine actual frontend rendering groups inspect model, population
badge and text instances through unknown/known transitions, local/foreign reveal,
discovery, a missing row, unavailable authority and explicit inspector reveal.
They exercise Hide, renaming, artwork and coordinate updates while only a name
is visible. All187 affected native groups pass. Five changed/affected production
TUs pass correctness analysis; the nine focused groups pass with Base.cpp,
BaseManager.cpp, frontend Game.cpp, InstancedText.cpp and Backend.cpp instrumented
under ASan/UBSan. Other objects are uninstrumented; leak detection is disabled.

A normal1024x768 Linux/software-rendered SMAC Planet startup reached the first
Base naming and research-priority flows, rendered Morgan Industries' model,
population badge and name, opened/closed its Base screen, and exited through a
normal window-close request with status0. The runtime reported no tracked
memory/OpenGL leaks; this is not a complete heap or performance validation.
The screenshot working tree is based on95c74e25 plus this PR. Immutable media:
`bba042b49d5f4bc09abd8b465d3714af0b85b437/base-disclosure-live/`.
