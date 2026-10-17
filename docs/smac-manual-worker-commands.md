# Canonical manual worker clicks

A canonical Base's resource grid now sends its actual tile click through the
local event queue to native worker allocation. The two existing event names,
`work_base_tile` and `unwork_base_tile`, represent an ordinary click for this
path. The backend inspects selection after its preliminary recalculation; a
stale UI label cannot override a repair that just selected the clicked tile.
Legacy Bases retain their existing script handlers.

The caller authenticates the registered Base, current-map tile and local
single-player owner both when queued and when dispatched. It accepts only the
Base and tile arguments. These two native commands do not execute registered
script callbacks. General event, movement, spawn and terrain fences remain in
place when worker knowledge is present.

## Calculation and publication

The implementation follows the ordinary-click branches in
[manual-worker-edit](behavior/manual-worker-edit.md) and their intake composition:
center reset; selected-square conversion using the actual automatic specialist
selector; consuming the last counted specialist; and manual replacement with
its separate signed score, ascending-index tie rule and initial minimum 999.
No food-sufficiency comparison or target-score comparison is added to a manual
replacement. Unavailable clicks still clear citizen management after preliminary
calculation. Other governor bits and saved riots remain exact.

Both recalculations use the existing complete allocator, actual worker
availability, current tile yields, frozen nutrient demand, owned orbital intake
and `GetCanonicalMineralSupport`. Support is read from actual saved same-owner
home links and retained SUPPORT. The manual replacement score consumes the five
frozen faction priorities, distinct from the automatic ungoverned-human branch.
All supported score stages are checked signed32 arithmetic; unsupported overflow
rejects before publication.

The preliminary result, edit and final allocation remain private. One existing
publication transaction prepares Pop projections, the reverse assignment index,
Base update queue and trigger set, then swaps the full allocation and context.
Pop addresses, IDs, variants and unused specialist slots survive. Queries do not
publish provisional workers. A later failure preserves the previous allocation,
context, indexes and queued presentation.

This is ordinary local human allocation with governor inactive. It does not
implement worker/specialist drags, the specialist type menu, editor/network
permissions, a resource cache, full nutrient/mineral/energy/citizen calculation,
or turn income posting. The complete original handler runs those broader
calculations; this route implements its allocation consumer and refreshes the
existing Base presentation. Existing canonical map, resource, facility and
sixteen-specialist projection limits still apply.

## Priority source and wire compatibility

The frozen faction profile now owns signed32 aggression/Explore/Discover/Build/
Conquer in that semantic order. They come from zero-based faction-header columns
7/11/9/10/8. The startup handoff's definition-to-current-field copies establish
the semantic mapping; the retained startup checker supplies these header fields
before executing that copy. This change does not claim a new execution of the
original faction text loader. GLSMAC uses strict twelve-column ingestion, an
optional trailing comma, and complete signed32 decimal conversion.

The current admitted campaign has no runtime personality editor. The frozen
initial values therefore supply current worker priorities; a future editor must
introduce its own saved mutable authority. Display names are not consulted.
Faction profile V3 appends all 35 values, GlobalSettings V12 transports them, and
World V23 binds complete equality. World V22's saved Unit homes and Base riots
remain intact. Older versions reject. Existing aggregate packet and world-field
budgets still apply before publication/copy.

## Focused validation

The cached native tests exercise the actual resource-grid script, real GSE event
wrapper and native dispatch, stable Pop/reverse-index/frontend updates, Doctor
retention, replacement of a better tile, center reset, unavailable clicks,
post-preliminary selection, and atomic late rejection. A saved home comparison
uses forest M2/intake M3 and aggression 500: support0 permits replacement below
999; actual support2 prevents it. The event preserves home, riots, knowledge and
both RNG states. Profile tests cover all five field positions, signed bounds,
late codec values and old-version rejection; existing host/world equality tests
cover the appended values.

The final cached native run passed all 39 affected groups with allocation
tracking. Targeted LLVM15 analysis of the seven changed production translation
units reported zero failures. No full platform run or new original-executable
run is part of this focused change.
