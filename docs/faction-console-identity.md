# Retained faction identity in the bottom console

The information page now uses the local Player's faction through the existing
`Game.get_player()` and `Player.get_faction()` APIs. The controller's roster
name (`Player.name`) is not the leader's identity and is never substituted for
it. No existing custom-leader field was found in the current roster model.

Fresh configuration imports display identity from the selected faction text:
the faction section's header field 0 is the full faction name and field 5 is
the default leader's name; the first field of its seventh record is the title.
The University excerpt therefore yields University of Planet / Provost Zakharov,
independently of the session controller's name. The existing short port name
and stable faction ID remain available for their existing callers.

This presentation metadata is optional and retained in Faction snapshot V2.
Each of its three fields is nonempty, single-line, NUL-free and at most 256
bytes. V1 records load without display metadata; the UI shows the retained short
faction name and an unavailable leader. It never fills an old save from changed
installed faction files. Explicit retained names survive save/reload verbatim.
Failed late metadata reads leave the destination Faction unchanged.

The information page is the initial center-console page; M and I still select
the message and information pages. Identity refreshes on page activation,
turns and player updates. Font, color and layout remain provisional pending
the original lower-console text contract.

## Ranking and graph boundary

`SmacSimulationState::ranking_scores` is one optional eight-slot table, not a
time series. `Game::PrepareCanonicalRankOrder()` derives the currently admitted
ordinary ranking from retained population/design aggregates and technology;
`AdvanceCanonicalTurn()` replaces the ranking/order before owner upkeep.
`SmacRankOrder` retains rank identities and tie ordering. Neither structure
contains historical samples, a graph sampling cadence, or the console's might
and integrity adjective mapping. Old missing inputs remain absent.

Consequently the console labels integrity, might and history unavailable. It
does not turn a current ranking into invented history or equate a dominance
predicate with the displayed might rating. The next graph slice needs the
original sampling, scaling, visibility and persistence contracts.
