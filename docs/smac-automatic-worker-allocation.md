# Detached automatic worker allocation

`SmacAutomaticWorkerAllocation::Allocate` composes the original worked-square
collection, specialist bookkeeping, worker scoring and allocation steps into
one owned result. It accepts a resolved unavailable mask, a frozen specialist
catalog, production and governor/faction priorities, existing specialist slots,
and intake already produced by the caller's orbital/prefix rules.

The result contains the cleaned old selection, final visited squares and gross
intake, the retained/provisional specialist state immediately before final fill,
and the final specialist count/types. It does not change a live Base, assign
population identities, publish events or consume gameplay RNG.

## Original control flow

The source is the worker/specialist section of original `0x4fd650`, pinned to
executable SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Managed allocation means AI, or a human with both the governor and citizen
management active. Human priority selection still uses the separate governor
flag. Managed upkeep bypasses existing-square collection and the initial
specialist split. Ordinary collection returns directly to final fill when
remaining slots equal retained plus provisional specialists, even if food is
short or another square would score better.

Later allocation compacts specialists before collecting a missing center. That
center query sees its visited bit unset; ordinary collection marks before its
query. Each candidate scan resets ranking and raw maximum nutrient yield,
traverses indices 1 through 20 in order, and includes visited squares only when
the worker/specialist budget is full. Clipped coordinates are skipped. Merely
scanning a sea square does not set sea participation.

Specialist competition can terminate a full-budget scan without insertion or a
tile swap. Tile additions, accepted swaps and rolled-back swaps query yields
afresh. Every swap ends scanning. Final fill uses its separate prerequisite-only
selector and writes from retained count to remaining slots, including the
original sixteen-position storage boundary.

## Contract and integration boundary

Population and specialist counts are admitted in `0..127`; nutrient consumption
must be positive. The stage APIs reject unsupported decision-arithmetic
overflow. Collection and missing-center intake use the original signed32 wrap.
Final specialist fill retains the stage's explicit positive-winner admission;
it does not silently extend the unobserved nonpositive winner domain. An actual
read beyond the sixteen stored positions needs an authenticated initial-selector
choice from the caller.

The yield callback returns a whole tile. It must not publish live state or depend
on the original resource selectors' individual side effects. Exceptions leave
the supplied input unchanged. Candidate scans, additions and rollback are
distinct fresh calls, so a caller must not substitute cached scan yields.

The companion tests project 31 named full-routine outcomes from the original
intake, cleanup, finalization and priority reports. Only recorded outputs are
asserted as original observations; short reported type prefixes do not imply
that all unused packed positions were observed. Separate adapter tests check
callback timing, fresh queries, sea participation, clipping, repair and failure
propagation against the static control-flow contract.

This allocation slice excludes the orbital prefix, final resource replacement,
support/maintenance, energy/psych distribution, production/research posting and
ecology tail. Live integration also needs saved exploration, remembered map
data, recognized ownership, visible units, convoy orders and directed faction
relationships. It must replay the accepted reveal/move/founding observations
instead of fabricating an explored or owned overlay. No default campaign or
manual worker command is enabled by this module.
