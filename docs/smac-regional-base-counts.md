# Retained regional Base counts

The [original producer receipt](behavior/restart-input-lifetimes.md) establishes
128 byte counters per faction. Starting/running founding increments the site's
region; starvation and Colony abandonment decrement it. These are byte deltas,
including wrap when the retained value is stale. Capture leaves both owners'
arrays unchanged. The existing post-Base owner summary clears and recounts only
its own array alongside its population aggregate. Nonzero reinitialization
preserves the array rather than replaying the fresh faction reset.

The native founding/removal transactions prepare the replacement array before
publication and apply it before callbacks. Initial founding uses its already
prepared topology and private Base candidate. Ordinary owner summaries use the
saved current topology. No additional region flood fill or peer recount runs.

Economy V3 stores the optional 128 raw bytes. New factions own a cleared array;
V1/V2 saves migrate with the field absent. Load and actual Start retain absence
or exact stale bytes. A later owner summary with topology can establish that
owner's current array. Deltas preserve absence until then. A summary without
topology cannot establish regional history and leaves it absent.

This connects the regional input writers required by the original restart
search. It does not enable that search, replace the existing placement policy,
or claim that fertility, ocean-choice and site-rating cache ownership are done.

Validation: cached Linux Debug build; 51 affected regional/economy/simulation,
capture, founding, last-Base, abandonment, starvation and summary groups with
memory debugging, plus ten existing startup/first-Base integration groups.
The first-Base fixture checks the fresh regional increment at actual creation
across difficulties, owner packages and claimed/queued Start. Tests distinguish delta updates from recounts, retain stale
counts through capture/load, exercise byte underflow and reject malformed array
framing. LLVM15 correctness analysis passed seven changed/affected production
translation units in 9.99s. Scalar state and transaction writes add no ownership
changes, so existing removal sanitizer results are reused.
