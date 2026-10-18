# Retained land-region concentration

The regional strategy threshold consumes a saved secondary status bit, not a
fresh terrain query or the identically numbered Standard Rules option. The
[stock-SMAC receipt](regional-state256-lifetime.md) and its
[22 original controls](regional-state256-lifetime-cases.json) establish the
separate reset, builder, faction-initialization and restore lifetimes. SMAX
behavior remains unverified.

## Owned state and arithmetic

`SmacSimulationState::retained_land_region_concentration` preserves three
states: clear, set and unavailable. Simulation V22 writes the optional Boolean;
V11–V21 retain explicit absence. Actual new-world simulation initialization
clears the value, independent of a previously generated map. Ordinary loading
does not rebuild or reinterpret it. Existing Base and faction-disposition
writers leave it alone.

`GlsmacTerritoryTopology::LandRegionConcentrated` supplies the measured builder
reduction: read stored area slots1–62, sum them, find the largest and compare
`largest >= floor(4*total/5)`. Unused retained entries participate; polar63,
region0 and sea64–127 do not. Empty and small totals retain the native inclusive
and rounding behavior. Missing area history and negative normal-region areas
reject. Wide intermediate arithmetic avoids C++ signed overflow; this does
not certify original overflow behavior for extreme diagnostic counts.

This is the saved-input and builder-arithmetic change. The current production
terrain action path does not invoke `GlsmacTerritoryTopology::Rebuild`; its
ordinary presentation refresh is a separate operation. Future gameplay actions
that actually execute the native region-builder boundary must publish this
result with their staged terrain/topology changes. This change deliberately
does not install a query-time or every-Base-event refresh. The regional
strategy consumer remains the next integration step.

## Validation

- Literal outputs from eight complete original region builders and six original
  area-reduction fragments, including stale unused slots and excluded regions.
  The fixture records the copied source JSON SHA256.
- Actual port startup clears the retained input even with concentrated displayed
  terrain. Both retained values survive founding, nonlast removal, owner-summary
  refresh and actual saved-world `Start`; missing legacy authority remains absent.
- Existing actual successful capture/restart and subsequent `Start` controls at
  turns6 and100 additionally check clear/set preservation. These port controls
  exercise positive faction2, not the original receipt's diagnostic owner0.
- V22 independent framing, exact false/true/absent round trips, V21 absence,
  truncated/trailing records, previous simulation codec and startup rejection
  groups: 42 affected native test groups pass in the cached Linux build.
- Correctness analysis passes for SimulationState, GameSimulation, topology,
  MapTerritory and Game translation units. Focused ASan/UBSan instrumentation
  of SimulationState and topology passes the same 42 groups; other objects
  reuse the normal build, leak detection disabled. No platform matrix or
  interactive rendering run was needed for this saved gameplay input.

The source receipt includes six original reset/restart prefixes and two
original ordinary file-body controls. Their entire executable call paths are
not claimed as ported by the fourteen arithmetic fixture cases.
