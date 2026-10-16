# Script run entry during a game or reset

`glsmac.run()` and the retained `set_smacpath` prompt callback both enter
`GLSMAC::S_Init`. That entry rejects an active game or a pending deferred reset
before loading resources, resetting the frontend, or replacing session state.
The existing guards on `init`, `deinit`, `connect`, and `start_game` therefore
cannot be bypassed by first calling `run`.

The supported `glsmac.reset()` path still schedules reset for the main loop.
The active-game flag is cleared after the backend reset succeeds and before
the main script is restarted. A failed reset preserves that flag and reports
the failure. Single-player map reload through `MT_Init` is unchanged.

Eight native `script run entry ...` cases call the actual `S_Init` method and
the actual retained path callback. They cover active/inactive and
pending/not-pending reset combinations, preserve lifecycle flags and the bound
State/Map on rejection, and retain the ordinary missing-assets prompt for an
inactive first run. The input is an empty temporary directory; no original
assets or UI are needed. These are entry-point tests, not rendered UI or full
public-wrapper tests. Return-to-menu and quickstart restart need a separate
rendered smoke check.

The tests against the preceding production code passed the two first-run
controls and failed all six active/reset cases (exit 1). With the guard, all
eight pass with allocation tracking. Linux Clang 15 Debug CTest passed 8/8 in
49.70 seconds; ASan+UBSan+LSan CTest passed 8/8 in 43.64 seconds with leak
detection enabled. The focused sanitizer run also passed with allocation
tracking, and the existing correctness policy passed the changed GLSMAC
translation unit. These are local measurements; the sanitizer cache retains
the previously documented local non-PIE workaround.

This guard does not provide the backend/owner transition coordination or
native reset/collector exclusion required by the separate lifecycle work.
