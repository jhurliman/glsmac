# RE-SMAC-CAPTURE-RETURN-STATIC-001: continuation after nonmodal Base show

**Static map only.** No successful screen, capture or movement return is newly
executed. The [frozen capture](undefended-base-entry.md) stops at Base-screen
entry; its [native continuation](captured-base-screen-close.md) completes the
screen's recalculation but stops at missing window construction. This document
maps what follows a normal mode0 screen return, without substituting that return.
[Machine-readable static map](capture-return-tail-static-map.json).

## Return chain

1. Mode0 skips the Base screen function's modal wait loop. Its show/construction
   and any earlier tutorial/presentation branches must still finish. Normal
   screen return removes its two arguments.
2. After that screen call, the capture routine performs only its epilogue and
   returns. There is no further capture-side recalculation, booty, facility
   lottery, technology award, production/worker edit or selection change.
   The caller does not test the screen/capture return value.
3. The movement body reloads its actor from the capture-tracking/remapping slot,
   updates its saved actor argument, and rejoins ordinary post-move processing.
   This tracking slot is separate from the primary selected Unit ID.
4. For the covered ground Scout, the body rejoins the ground post-move path,
   refreshes sight, passes applicable bookkeeping and reaches its shared exit.
5. The local, non-network wrapper forwards the movement body's return value
   directly. Its multiplayer follow-up path is not used for this case.

The alternative capture branch that sets a global stop/return marker when the
map selection query fails is **before** the screen call. The frozen case reached
the screen branch instead; do not also apply the alternative marker update.

## Covered post-move route and writes

The following deductions use the frozen inputs: actor1 is owner1's ordinary
Gun/Scout Infantry, plan Recon, transport capacity0; both endpoints are land;
destination Base1 exists; the actor's faction is human and locally focused;
the cached destination site query is0. The supplied tile flags exclude the
post-entry monolith condition, and the site query's early suppression test
explains its zero result. These are static deductions, not additional observed
tail branches.

| Stage | Covered case | Other branches / limitations |
| --- | --- | --- |
| Transport follow-up | Capacity0 bypasses cargo order loop | Transport/sea branches can clear orders and change selected Unit |
| Regional transition bookkeeping | Ground unit with land source rejoins ground tail | Cross-domain path can increment regional counters |
| Zero-strength/native handling | Positive Gun skips zero-strength stack reorder; owner1 skips native-owner healing | Other weapons/owners can reorder stacks or change damage |
| Sight refresh | Actual sight-refresh routine is called | Can update map/Base memory, Unit visibility/status/orders and encounter bookkeeping; not merely drawing |
| Air fuel/damage tail | Ground domain bypasses it | Air branch can spend/reset movement, damage, delete Units and invoke hit presentation |
| Monolith/site handling | Recorded site conditions bypass these branches | Other tiles can call special-site routines; no general RNG claim for those |
| Artifact/production handling | Recon plan bypasses Artifact branches | Artifact branch can change Base minerals/production baseline and delete its actor |
| Cargo arrival selection | Both possible positive assignments to the caller's cargo-arrival flag are excluded here | Nonzero flag can reorder/clear orders and change primary selection |
| Human/AI arrival split | Human actor bypasses AI transport-arrival handling | Other owners/orders can reset linked orders |
| Direction bookkeeping | Valid direction2 is stored and a movement-status marker is set | Invalid direction clears that marker instead |

No direct primary-selection store on the inspected post-capture route applies
to this ordinary Scout. There is no direct secondary-selection store in the
movement body's tail or the local wrapper. Conditional preservation of selected1
and secondary none follows **only if** the missing window and subsequent
callbacks do not change them. The transport, Artifact and cargo alternatives
contain primary-selection writes and must not be generalized away.

No additional position, home, population, owner, production, queue, worker or
treasury write is present in the selected top-level ground tail. Thus the last
observed actor(20,16)/spent3/home0 and Base1 owner/population/production values
are compatible with a normal return without edits. This is not proof that
all record bytes stay unchanged: sight and movement bookkeeping remain active,
and opaque UI/contact callbacks have not been executed through completion.

## Guards not fully captured dynamically

The movement body retains an entry Unit count and an internal result flag.
If Unit count changes, it takes the count-change cleanup exit and returns0.
With unchanged count and result flag1, it takes the successful cleanup path and
returns1. The normal positive-move/capture prefix sets that flag1. The local
wrapper then returns the same1. **This is a conditional static prediction,
not a new expected-output fixture or a recorded original return.**

Additional guards still matter:

- Actor order and automation/status bits can enter an activity counter/warning
  branch. At its threshold, a local confirmation helper may be called. Those
  raw bits/counter values were not part of the frozen semantic state.
- An arrival/status bit can conditionally be cleared after a further arrival
  query. The remaining local path skips multiplayer route/order updates.
- The screen's missing show methods, UI message dispatch and contact-related
  sight callbacks can affect state. They are not modeled as successful no-ops.
- Actor remapping is reloaded after capture. The observed roster has no actor
  loss or compaction, but post-screen mutation is not dynamically established.

The selected top-level ground blocks, capture epilogue and local wrapper contain
no direct CRT/shared RNG call. This does **not** establish a zero-draw budget
for all remaining work: window construction/show, visibility/contact helpers,
message dispatch and optional warning callbacks are not fully audited
transitively here. The last observed RNG states remain CRT1951609278/shared3;
no final RNG state is fabricated.

## Separate close / hide path

A common Base-window close helper is directly identified. It clears the open
flag, releases an owned display resource, removes/hides the Base window from
the UI focus stack, polls UI, and conditionally restores the map view using
stored view coordinates. It reads the selected Unit's position for the view
restoration guard; it does not directly assign either selected Unit ID.

The constructor's embedded window-interface table also identifies a hide
lifecycle callback. Its hide branch tears down child views, adjusts display/
audio/UI state, and yields to UI dispatch. Neither inspected close/hide body
directly invokes Base resource recalculation, production switching, citizen
assignment, booty, technology acquisition, Unit movement or RNG.

This is narrower than proving every close interaction harmless. The exact
Done/Escape event route is not fully bound here, and the generic focus-stack
dispatcher invokes window activation callbacks. Other event routes can edit
gameplay. Mode0 return does not itself invoke this close helper, and a port
must not require closing the nonmodal Base screen before continuing movement.
