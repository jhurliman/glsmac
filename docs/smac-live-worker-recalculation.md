# Live ordinary worker recalculation

`Base::RecalculateSmacWorkers` connects the existing automatic allocator to an
actual canonical Base. The script callable is
`base.recalculate_smac_workers(worker_id, specialist_ids)`; the second argument
contains seven registered positional population-definition IDs. It runs in the
existing writable owning accumulation and enqueues the normal Base refresh.

The caller requires the local single-player human, an inactive governor, saved
allocation authority, and ready owned worker knowledge. It reads population,
turn, the positive frozen nutrient rate, all 32 selected bits and all sixteen
specialist slots. Availability comes from the actual entity/terrain projection;
each yield request resolves the measured radius and calls the existing ordinary
canonical tile-resource adapter. Specialist availability uses the bound owner's
actual technology. Orbital intake comes from the saved faction counters through
the live orbital query. The complete current Unit projection must consist of
ready conventional units with `Home::None`, establishing zero mineral support.

The saved production kind supplies the scoring category. Unit, Facility and
Stockpile are admitted without interpreting their opaque target strings;
Orbital, Project and missing production reject. This establishes neither target
buildability nor completion behavior. Human governor-off priorities are zero by
the measured priority branch, independent of omitted AI personality inputs.

All calculation remains detached until the existing complete allocation
publication prepares the Pop projection, reverse assignments and update queues.
The final visited set and specialist slots replace allocation together. Stable
Pop identities and variants survive. A valid complete Doctor allocation is
retained even when a productive tile is available; this is not a reset or
force-optimize command. Errors during queries or publication preflight leave
authority untouched. No temporary allocation is exposed to script callbacks.

This adds no general event exemption, upkeep scheduling, manual-click handler,
population growth or complete resource/citizen calculation. The current canonical
resource/map/facility admission and live allocation's unaliased-coordinate,
other-center, global reverse-index and sixteen-specialist restrictions remain.
Gross intake is read through the existing callback after publication. Complete
canonical consumption remains unavailable; no mineral/energy balances or stored
resource posting are fabricated.

Focused tests use real Base/Unit/Map authority with supplied terrain/knowledge
and synthetic frozen source. They exercise repair and subsequent actual intake
and frontend refresh, valid-specialist retention across the three production
kinds, early/yield/late-publication rejection, and actual world restore followed
by the same next recalculation. They do not execute original code or claim a
generated playable campaign or rendered base-screen flow.

Validation: the four focused native groups pass with allocation tracking on the
cached Debug build. LLVM15 correctness analysis of `BaseWorkers.cpp` and the
changed Base wrapper reports zero failures. The initial restore test used an
in-place fixture clear that correctly hit the knowledge mutation fence; the
final test uses a separate actual decoder destination. No production exemption
was added, and no full-suite/platform rerun was required for this caller.
