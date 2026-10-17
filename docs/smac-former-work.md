# Ordinary local Former work

The live Unit now owns a work order and progress byte. Unit record version 4
preserves both through movement and exact restore; cancel clears only the order.
The enum is a port wire identity mapped explicitly to the frozen TERRAIN source
rows, not an original order-byte encoding. Non-Former packages reject work state.

Native `work_unit`, `continue_unit_work`, and `cancel_unit_work` events authenticate
the actual local registered ready Former, its tile membership, frozen prerequisite,
and supported no-project campaign. Request and dispatch remain separate. A work
dispatch with positive movement remaining pools matching same-owner progress with
byte wrapping, spends only the actor's full allowance, and adds ordinary rate 2.
Completion clears matching orders, changes actual improvement bits, and refreshes
the acting owner's remembered improvements. It does not spend either RNG stream.

The admitted command terrain is flat land without features, a Base, existing
forest, or special improvements. Farm, Road, Forest, Solar and Mine use active
frozen work times; repeated Farm/Road upgrades, forest-removal awards, sea work,
special abilities/projects, foreign stacks and broader legality remain outside
this implementation. Calculator terrain adjustments do not grant command admission.
The existing ordinary movement adapter accepts completed Farm/Solar/Mine bits,
so producing an improvement does not trap the Former.

All Unit records, update queues, remembered knowledge and the semantic tile-update
request are prepared before the first authority write. Commit has no scripts or
allocations. The metadata request refreshes actual tile/preview data; this source
does not rebuild world terrain actors or atlas graphics. Actual canonical Base
intake reads the completed improvement on its next query without changing worker
allocation or posting resources.

The provisional GLSMAC keys F/R/O/S/M assign the corresponding action; C cancels.
They do not invoke work themselves. The active frontend selected-unit controller
now queues saved work automatically. A transient visit latch permits one request
per selection/turn or Unit presentation update; the backend reads the saved order
and allowance. Session-bound callbacks retain copied IDs and a weak visit token,
so selection changes, manager destruction and session retirement cancel delayed
visits. Every `continue_unit_work` event carries its captured `turn_id`, checked
again before wrapper access at dispatch. Old events cannot consume a later turn's
refilled allowance. There is no new Continue key or work loop in faction upkeep.

The `former-human-controller` and `local-unit-selection` receipts now feed the
native automatic selection query. It uses current saved Unit record order,
integer wrapped-X distance, and current-selected identity to break equal-distance
ties. The enclosing controller retains an eligible current Unit before scanning. It
also holds a current persisted work order after dispatch exhausts allowance, even
while another Scout is ready. Cancellation/completion releases this held order.
That logical selection survives upkeep, keeping the working Former current when
both Units refill. Copied IDs
and a session-bound weak result cross the backend/frontend boundary; presentation
updates retire and reissue pending requests. The frontend waits for the selected
Unit's ready presentation rather than choosing according to request arrival order.

This route admits the existing local conventional runtime states. It does not
invent Sentry, convoy or skip-marker/button state. Camera-to-tile focus conversion
and manual Tab cycling remain explicit GLSMAC input policies; the native query
accepts the reference/focus coordinates separately. Full original outer-loop,
modal and multiplayer selection policies remain outside this change.

## Evidence and focused checks

The accepted `former-full-actions` receipt binds public input SHA256
`9e1b85c05165d6193d6fdc23cb00419b453b18e4663796e3a834b9dbded25899`
and retained original report SHA256
`9bd6ad28b24209986cc053c6284666b31b637201c4ec17aec37fab9c93f98a28`.
The original executable is `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The five solo work times and three-worker progress/unused-helper sequence are
literal expectations from that receipt. Cancellation/movement follows the earlier
`former-command-loop`, `former-directional-input` and `former-cancel-and-activate`
contracts. Byte-wrap and generic replacement follow the separate work/replacement
contracts. This is not a rerun of the original executable or all nine sequences.

Cached incremental build and 20 tracked native groups pass: seven new work groups,
five terraforming catalog groups and eight existing manufacture/home/record groups.
The live groups exercise actual command dispatch, real end-turn movement reset,
restored continuation, Base intake, same-turn repeat prevention, three-worker
collaboration and atomic rejection. The synthetic restore initializes asset-free
preview coordinates before applying terrain presentation updates. Eight changed
production translation units pass LLVM 15 analysis (19.71 seconds). No full suite,
platform matrix, rendered-game or automatic-controller claim is made for this source.

The selected-controller follow-up adds two actual native regression groups: real
work/upkeep with exact event-count latching and delayed callback retirement, and
an old stamped visit queued behind real upkeep. The asset-free controller fixture
supplies selected identity to the same private pump used by frontend Iterate; it
does not assert the existing renderer/selection policy. Four affected production
and fixture translation units pass syntax checks. Runtime execution of these new
groups is reserved for the shared cached integration build.

The selection follow-up adds two groups. Reduced literal controls from the
20-control selection receipt distinguish distance ties, current-reference origin,
wrapped-X distance, partial allowance and the integer axis metric. The live
record-order check creates a later Scout with a smaller numeric ID. Four actual
Former work visits span three real upkeeps without re-exhausting the competing
Scout; each immediate post-work query checks held identity and final completion
checks its release. The asset-free fixture exercises the frontend's request helper and native
result, then supplies accepted identity to the existing work pump; it does not
claim rendered selection acceptance. Four affected translation units pass syntax
checks. These two groups await the shared cached integration run; no full build
or platform run was performed for this follow-up.
