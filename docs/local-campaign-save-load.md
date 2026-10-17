# Local GLSMAC campaign files

The native .glsmac V1 envelope saves the existing complete world snapshot,
frozen settings/faction metadata and exact local seven-slot roster. It does not
read or write original SMAC .SAV files. The first local file policy supports a
SMAC campaign with the local Single player in slot zero and additional human
Player participants. Network sessions and unstable turns/queued commands reject.

Saving validates before touching the destination, exclusively creates a sibling
temporary file, writes and flushes it, then atomically replaces the requested
path. Exclusive creation prevents concurrent processes from sharing a temporary. File and nested
frame budgets are checked before materialization. Loading stages a new State,
checks the exact frozen profiles, roster and complete world envelopes, then uses
a claimed RestoreCampaign owner checkpoint. Normal configure runs once; the
saved world replaces definitions and entities. Initialize regenerates renderer
recipes and InitComplete binds deferred homes/knowledge. It never calls new-world
creation, entropy initialization or FirstTurn. Camera/selection use normal initial
presentation policy rather than saved screen coordinates.

Main-menu loading preserves the previous menu State until frontend initialization
succeeds; cancellation/initialization failure returns through the existing reset
path. Malformed file/prefix input throws before publication. Existing running-world
load is refused. The load API returns after acceptance, with normal asynchronous
loading afterward.

Saving is asynchronous: `game.save_campaign(path)` enqueues only owned path data
and returns a nonreused request ID. The backend GAME-thread Iterate checkpoint
claims the request and publishes an existing OwnerTransition. The session-bound
GSE owner accumulation then validates and serializes the world and writes the
file. While that accumulation is pending or running, OwnerBlocksNative prevents
backend iteration from mutating the world; the accumulation also serializes with
already-extracted script event callbacks. This is an ownership handoff, not just
a lock around the pending-event container. Raw synchronous SaveCampaign is private
and rejects calls outside the claimed operation.

The frontend polls an owned result and emits `campaign_saved` with
`{request_id, path, error}`; success includes the absolute path only after atomic
replacement. The popup waits for that result, suppresses duplicate submits, and
ignores unrelated or retired request IDs. Validation/write errors leave the
campaign active and permit retry. Reset cancels the session-bound operation and
clears its result, so a queued old-world save cannot write or notify a new world.

Validation: affected production and test TUs syntax-checked. At integrated
`d2fbf105`, the repository LLVM 15.0.7 correctness policy passed all four explicit
campaign production TUs (`CampaignFile`, `GameCampaign`, `StateRoster`, `GLSMAC`)
in 10.34 seconds. Three native groups
cover file replacement, actual claimed restore (the asset-free fixture skips
renderer construction), settings/roster/world authority, subsequent turn, malformed
file session preservation, queued-command rejection and partial frontend cleanup.
The restore comparison normalizes only unordered PopDef registry enumeration; it
checks each definition ID/value and preserves exact comparison of all other world
bytes, including Base and Unit record order.

The complete file/restore/next-turn/callback-failure group passed with allocation
tracking at integrated `f0c4d8af` after its viewport fixture correction. The
cancellation and malformed/unstable-save groups passed at `d2fbf105`; their code
is unchanged at the final integration. The real GSE `campaign_file_ui.gls.js`
module test passed at `20f06c31` and is also unchanged. The integrated cached
build linked successfully. These are focused Linux receipts, without a full-suite,
platform-matrix or rendered acceptance claim.

Owner-request amendment: the cached Linux build linked at integrated
`c14d32c7cba4d88f98470e232f25a9aa26ef6fd2`; the three existing campaign groups
passed with allocation tracking, and the updated real Save/Load module script
passed. The new ownership group initially reused a probe that ran during both
validation and apply; its apply-only fixture correction was rebuilt at
`9cd97fac5e8df10f018f45387f628d73c121d736`, where that group passed with allocation
tracking. It covers an actual extracted event, deferred backend claim, complete
post-event bytes, exactly-once frontend delivery, failed-save recovery and
pending-reset/stale-callable retirement. The prior three groups were unchanged.
Three changed production TUs also passed focused LLVM 15 correctness analysis
(zero failures, 17.56 seconds); two test TUs passed syntax checks. Logs are
`campaign-owner-{build,rebuild,native,concurrency,ui}.log` under the shared Linux
cache parent. Earlier receipts above describe the pre-amendment implementation;
no full suite or platform rerun was performed for this amendment.
