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
They do not invoke work themselves. Native continuation is the mechanism for the
human selected-unit controller, whose new original receipt establishes automatic
selected-unit work. The frontend scheduling/latch connection is a separate follow-up;
there is no new Continue key and no work loop inside faction upkeep here.

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
