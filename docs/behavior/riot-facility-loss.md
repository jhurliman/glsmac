# RE-SMAC-RIOT-FACILITY-001: riot consequence gates and facility loss

Publication scope: this repository includes this handoff and its literal cases. Other research notes named as unpublished follow-ups below are outside this published evidence boundary and do not expand its claimed scope.

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 23 named and 1,300 sampled cases pass. They execute the human
riot handler through early exits, actual facility removal, or entry to the
later unsuccessful-search branch. Random returns are supplied; notification
and map callbacks are omitted without interventions. Defection and facility
removal aftermath remain outside scope.

## First-riot grace and ongoing gate

The handler first sets the riot-status flag and detects whether this is a new
riot. In the checked human path, a newly starting riot does not proceed to
the random consequence gates during that invocation. An ongoing riot does.
The initial notification callback can affect the full game's response; these
checks assume it leaves state unchanged. AI governor handling is not tested.

For an ongoing riot with current POLICE R:

- If R>-6, draw a random return and proceed only when `draw % (R+7) == 0`.
- If R<=-6, proceed without consuming this draw.

These are exact remainder conditions for supplied nonnegative draws. The
random generator and its distribution are not tested, so do not infer an
exact uniform probability from these comparisons alone.

After that gate, stop this consequence path if any of these hold:

- Completed Headquarters is present at the current base.
- The faction's base count is at most one.
- The unsigned conquest-assimilation counter is at least 45.

The POLICE draw occurs before those protection checks and is consumed even
if one subsequently stops the path. The assimilation counter 44 permits
continuation; 45 and 255 stop it. This is a counter threshold, not a newly
established capture grace duration.

If population is below 4, consume another draw and proceed only if
`draw % 3 == 0`. Population 4 or greater skips this additional draw.

## Facility search

Let the search limit be:

`L = population + 20 - difficulty - faction_rank`

The faction rank is a resolved runtime strength rank; its complete builder
and update timing are not part of this handoff. Difficulty is the stored
faction difficulty read by this handler. Tests cover normal ranks 1..7 and
difficulties 0..5.

Start attempt index at zero. For each candidate, consume a random return and
take its remainder modulo 39. A candidate can be removed only when:

1. Its installed facility ID is in 2..33 inclusive.
2. That completed facility is present at the base.
3. It is not named by the faction's explicit FACILITY grant modifiers.

Rejected candidates consume attempts. The first eligible candidate at attempt
index at most L is removed immediately by clearing its stored presence. Thus
normal nonnegative L permits L+1 candidate draws. The checks distinguish an
eligible candidate at the final allowed attempt from one just beyond it.

Candidate 0/1 and 34..38 never cause removal through this search. In installed
SMAC, Psi Gate is ID 33 and can be removed; Sky Hydroponics Lab is ID 34 and
cannot. The fixture's facility-selection order records the installed list.
Headquarters also has its earlier whole-path protection.

A faction FACILITY grant protects its matching facility from this removal
predicate. A protected selection is retried; it does not end the search.
This is not a general rule protecting every facility obtained for free from
any source. The routine tests explicit matching faction grant modifiers.

At most one facility is removed by this branch per invocation. These checks
stop immediately after removal, before formatting and the facility-loss
notification. They verify the stored presence change itself.

## Unsuccessful search and open consequences

Exhausting the attempts enters a later branch instead of removing a facility.
That branch can consider other riot consequences, including faction changes;
it is not executed in this family. Do not interpret `reached_post_search_branch`
as proof that nothing else happens during the riot.

The [23 named cases](riot-facility-loss-cases.json) cover first-riot grace,
POLICE draw/bypass, HQ/base-count/assimilation gates, small-population draw,
free-grant protection, retry/modulo behavior, the facility range and inclusive
attempt boundary. Sampled cases combine those inputs across populations up
to 127. Comparisons include consumed random-call count, candidate attempts,
removed/remaining facilities and the later-branch boundary.

Original gameplay predicates, bit helper and removal instructions execute
unmodified. RNG returns and nonmutating notification/map callback returns
are supplied. Full RNG behavior, AI, notification interventions, defection,
other riot consequences, rank construction, capture initialization and whole
turns remain outside these removal checks. Subsequent
defection recipient selection (unpublished follow-up `riot-defection-recipient.md`) now has separate
coverage; actual ownership transfer and its cleanup remain open.
