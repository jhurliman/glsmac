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

## Next three executable slices

1. Connect automatic selection to the value context: ascending IDs0..88,
   ordinary enabled/unowned/prerequisite admission, native depth scaling,
   Explore/Build Formers early return, wrapped bounded-range draws and strict
   winner comparison. Reproduce target and final shared RNG state from the
   original selection/dispatch receipts. Do not use the manual category/modulo pool.
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

The reverse engineer is checking the retained regional strategy/attack/defense
producer needed by slice2. Existing reference receipts supply valuation and
selector/dispatch arithmetic; no placeholder live producer is authorized by them.
