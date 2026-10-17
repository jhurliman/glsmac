# Local GLSMAC campaign files

The native .glsmac V1 envelope saves the existing complete world snapshot,
frozen settings/faction metadata and exact local seven-slot roster. It does not
read or write original SMAC .SAV files. The first local file policy supports a
SMAC campaign with the local Single player in slot zero and additional human
Player participants. Network sessions and unstable turns/queued commands reject.

Saving validates before touching the destination, writes a sibling temporary
file, closes it, and atomically replaces the requested path. File and nested
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
load is refused. The save API returns the absolute path after successful replacement;
the load API returns after acceptance, with normal asynchronous loading afterward.

Validation: six affected production TUs and three test TUs syntax-checked. The two
native groups cover file replacement, actual claimed restore (asset-free fixture
skips renderer construction only), settings/roster/world bytes, subsequent turn,
malformed-file session preservation and queued-command rejection. Linked focused
results are pending the shared cached build; no platform/full-suite claim.
