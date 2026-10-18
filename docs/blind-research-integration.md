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

## Next three executable slices

1. Connect the tested selector/value composition to the live request with
   resolved catalog identities and the owned gameplay random state. Do not use
   the manual category/modulo pool for Blind Research.
2. Add persistent faction strategy priorities and live input producers. Initialize
   priorities from the faction header; preserve non-Boolean values on cancel.
   Bind regional strategy/Base/known-tile summaries, maximum attack/defense,
   ecology aggregate and endgame-pressure authority at the original turn stage.
   Missing producers must not silently become zero or a later snapshot.
3. Connect the pending priority UI and turn continuation. Request the first
   target only after a positive research submission, including no-target paths.
   After acquisition, choose the replacement with new ownership before the
   outer lab/threshold reset; preserve Secrets continuation. Save/reload while
   suspended must resume once with identical priorities, target, labs and RNG.

The original ordinary local priority question is admitted for turn<=11 or
trunc(historicalTechnologyScore/2)%5==1, absent an inhibiting shortcut. An empty
priority mask is valid; cancellation retains existing integers and still selects.
The automatic chooser consumes the shared bounded-range stream, not the CRT stream.

The reverse engineer has established the retained producer needed by slice2.
The legacy `maximum_defense` valuation input actually means the first qualifying
relationship tier's retained foreign weapon capability, not the owner's armor.
The owner's available weapon capability is refreshed from catalog/technology
eligibility before the owner summary scans units. Strategy codes are rebuilt
later, after research/Base processing; startup strategy0 and post-summary7 are
distinct retained states. Preserve that stage boundary and saved values rather
than recomputing a convenient fresh summary during selection. The remaining
regional/ecology providers must be connected without placeholder zero inputs.
