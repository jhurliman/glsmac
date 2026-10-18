# Ordinary human Blind Research integration

Standard Rules enables Blind Research. The current live research request still
uses the ordinary manual pool, so that option is not yet faithfully implemented.
The fix must use the original automatic selector and preserve its input lifetimes,
shared random stream, asynchronous priority question and discovery continuation.

## Complete valuation

`SmacResearchValue` implements the complete ordinary human mode-zero calculation
specified by RE-SMAC-BLIND-RESEARCH-001: weighted priorities, regional power,
owned early return, endgame pressure, PLANET/PROBE, Explore/Build, Secrets,
PSI/reactors, ecology, population, weapon records, Secrets ancestry, Formers and
Foil/region adjustments. Signed32 additions and multiplications wrap explicitly;
each division truncates separately. No blanket minimum clamp is added.

`SmacHumanResearchInputs` is a resolved query context, not faction state and not a
set of fallback values. It separates installed P/T/W/G ratings from mutable
Explore/Discover/Build/Conquer priorities, raw possession from enabled ownership,
and retained regional/ecology inputs from fresh world calculations. Facility,
reactor, chassis and design references come from their catalogs; fixed original
technology comparisons remain fixed. Arbitrary cyclic custom graphs and extreme
signed-division traps are outside the measured domain.

The native fixture records 107 independently executed original-code results:
all 89 installed technology IDs plus 18 focused controls, with 17 distinct
retained input contexts. The numeric receipt's G/T/W/P ratings are explicitly
projected to catalog P/T/W/G. The compact test retains all required inputs and
expected values without original executable/asset payloads. Its source-receipt
hash is recorded next to the fixture. The same public cases pass a focused
ASan/UBSan invocation of production code.

This completes the value dependency; it does not yet change live research
selection. A matching kernel is not evidence that the gameplay option works.

Twelve additional [native review discriminators](behavior/human-research-review-discriminators-cases.json)
confirm that weighted Power matching uses forward Vendetta bit16, distinct from
the later regional comm-link bit8. Hunter-Seeker ancestry uses
`1+bool(Discover!=0)`: Discover 2 and -1 both use budget2. Three-edge and
four-edge controlled chains distinguish that behavior from `1+Discover`.
The public test now checks all 119 original results.

## Persistent priorities

New-world admission now copies the four frozen personality priorities into
owned faction strategy records in Explore/Discover/Build/Conquer order. These
records are separate from technology ownership, Base governors and immutable
definitions. They preserve full signed values. A canceled priority question
leaves them untouched; accepting any four-bit mask, including zero, stores
exactly 0/1 per axis.

Simulation format V16 retains these records by simulation ID. V11-V15 saves
restore explicit absence because they never recorded priority authority;
restoration does not fabricate it from current definitions. This state alone
does not enable live Blind dispatch or suspended-priority save/resume. Those
callers must require the saved authority and preserve the turn continuation.

## Automatic selector

`SmacAutomaticResearch` now scans normal IDs in ascending order, admits enabled
unowned candidates with owned prerequisites, applies the original Blind depth
scaling and Formers early return, and consumes the supplied shared/CRT streams.
The value callback runs before the preferred-design check. Human zero-width
draws still advance the shared stream; AI nonpositive scores do not draw.
Wrapped products, strict winner comparison and the repeatable ID88 ownership
boundary are preserved. Cyclic custom prerequisite graphs reject explicitly;
the original controls establish ordinary acyclic definitions.

The 32 published original-selector fixtures verify target, valuation-call
order and both final random states. Six additional original chooser traces
compose the complete human valuation with the selector, including all priority
axes, an empty mask and retained non-Boolean priorities after cancellation.
These verify the actual valuation rather than supplying its results. The live
turn request still needs the retained producers and priority UI described below.

## Live capability producer

The [component binding receipt](behavior/capability-scan-bindings.md) and
[startup caller receipt](behavior/startup-capability-and-ecology-rng.md) now
back a live retained producer. Startup refreshes each admitted owner's caches
inside design generation, before marking/selecting designs. The later owner
summary refreshes them again; acquiring technology or opening an inspector
never refreshes them early. Nonzero faction initialization clears only that
owner's row.

The scan preserves the original signed-byte weapon/armor and unsigned-byte
land-speed inputs, fixed native-design seed components, separate PSI maxima,
and relationship-tier foreign maxima. All seven positive faction slots retain
signed32 results, including stale peer values. The current campaign admission
excludes projects, and its validated faction definitions supply the original
zero PSI scalar; broader project/faction authority remains future work.

Simulation V17 saves these caches without recomputing them. V11-V16 saves retain
explicit absence. Original ordinary saves do not serialize both gameplay RNG
words; deterministic paired-stream restoration remains a port policy.

Validation covers 31 native component/width controls, 15 native PSI controls,
actual research-choice save/Start continuation with delayed cache refresh, and
actual last-Base loss/restart/empty-owner initialization. Regional strategy,
ecology and the priority question still need live integration before Blind
research can use the automatic selector.

## Next three executable slices

1. Persist and connect the retained input producers described below. Priorities
   already have saved authority; regional strategy and ecology
   lifetimes still need their live writers; capability refresh is connected. Bind actual catalog identities,
   regional Base/known-tile summaries and endgame pressure. Missing producers
   must not silently become zero or a later snapshot.
2. Connect the pending priority UI and turn continuation. Preserve non-Boolean
   priorities on cancel and accept the empty mask. Save/reload during the
   priority question must retain the exact chooser context and resume once.
3. Dispatch Blind requests through the tested selector/value composition and
   owned shared random state, replacing the manual category/modulo pool.
   Request the first
   target only after a positive research submission, including no-target paths.
   After acquisition, choose the replacement with new ownership before the
   outer lab/threshold reset; preserve Secrets continuation. Save/reload while
   suspended must resume once with identical priorities, target, labs and RNG.

The original ordinary local priority question is admitted for turn<=11 or
trunc(historicalTechnologyScore/2)%5==1, absent an inhibiting shortcut. An empty
priority mask is valid; cancellation retains existing integers and still selects.
The automatic chooser consumes the shared bounded-range stream, not the CRT stream.

## Retained producer contracts

The [live territory summary integration](retained-territory-summary.md) now owns
the territory/known/suitable/covered/site counters and their scheduled refresh
and save lifetimes. Per-owner regional military/strategy and ecology integration
remain required before the automatic research dispatcher can use complete inputs.


The [regional producer receipt](behavior/regional-research-summary-producer.md)
establishes the capability and strategy update boundaries. Its native controls
cover 14 owner summaries, nine capability refreshes, two initializer prefixes
and one save/load round trip; all 1,792 checked strategy entries match the
independent decision model. This is reference evidence, not a port integration
test or a claim that arbitrary foreign-Base routing is implemented.

The legacy `maximum_defense` valuation input actually means the first qualifying
relationship tier's retained foreign weapon capability, not the owner's armor.
The owner's available weapon capability is refreshed from catalog/technology
eligibility before the owner summary scans units. Strategy codes are rebuilt
later, after research/Base processing; startup strategy0 and post-summary7 are
distinct retained states. Preserve that stage boundary and saved values rather
than recomputing a convenient fresh summary during selection. The remaining
regional/ecology providers must be connected without placeholder zero inputs.

The [PSI capability receipt](behavior/psi-capability-estimates.md) supplies 15
native controls for the missing general component estimates. They consume the
previous foreign weapon/armor caches, apply the configured offense/defense
ratio, then the faction PSI percentage and applicable owned project, truncating
at each step. Native-design eligibility seeds separate PSI caches only;
available negative-power components separately participate in ordinary maxima.
An inspector query or technology valuation must not refresh these caches.

The [ecology producer receipt](behavior/base-ecology-research-aggregate.md)
executes 20 complete owner passes across ten controls. Clear only the incoming
owner's aggregate before its Base pass. Each Base's research runs before its
final eco-damage contributes `trunc(4 * damage / clamp(4 - PLANET, 1, 99))`.
The first Base therefore sees zero; the second sees only the first Base's
contribution. The later owner summary preserves the completed aggregate.
Resource previews do not add contributions. The original ecology handler also
draws from its CRT stream even at zero damage; implementing the aggregate alone
does not establish event or RNG fidelity.

The ordinary low-mineral ecology control uses the original resource scan.
Positive controls provide resolved mineral intake and PLANET at the native
producer boundary and suppress native-life events after addition/RNG. They
establish arithmetic and ordering, not construction, blooms or climate events.
Cold startup zeroes are distinct from initializer behavior: poisoned strategy
entries survive the checked initializer. Saved history must not be fabricated
from current terrain, definitions or a newly calculated owner summary.
