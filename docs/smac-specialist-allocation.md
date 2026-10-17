# Owned specialist allocation stages

`SmacSpecialistAllocation` implements the specialist bookkeeping needed between
existing worker collection and final allocation. All inputs and results are
detached values. It does not change a Base, run a manual command, assign default
population, replace obsolete specialists, or produce their final energy output.

## Authority and API

`SmacSpecialistSlots` owns sixteen positional types, including unused padding,
and an explicit total count. `SmacSpecialistBookkeeping` owns those types plus
retained/provisional counts. There is no parallel live allocation authority or
new serialization format.

The caller supplies seven economy/psych/labs triples, prerequisite and obsolete
availability, and the configured minimum size. This module copies those values;
it does not resolve tokens or technology ownership. The typed catalog resolver
and its ownership test from the integrated source are deferred with the frozen
catalog and live query foundations. Callers must supply authenticated availability
and refresh it when ownership changes; this module does not maintain a cache.

The caller supplies an optional initial-overflow choice obtained from the
authenticated initial selector. Split/compaction require it only if they really
read a position at or above sixteen. A provided choice must be 0..6. Padding
and fallback are inputs; Doctor is not an invented universal default.

## Measured stages

`Split` counts exactly zero psych as provisional only for managed ordinary
calculation. Negative psych is nonzero. Stored types are unchanged. Unmanaged
and managed-upkeep inputs retain the original count with provisional zero.

`Compact` is called only when the caller reaches later handling rather than
collection's complete-allocation bypass. Managed inputs with retained count
above zero shift a zero-psych entry out, reading through the old active end.
Only the first sixteen destinations persist writes. Provisional count decreases
when nonzero; retained count does not. The index advances after removal, so an
adjacent zero-psych type can survive. If provisional is zero, a padding or
overflow type can become active without reducing the combined count.

`Compete` requires managed allocation, population at least five, turn after
100, and either nutrient intake strictly above demand or no positive candidate
nutrient return. The configured minimum size does not change that fixed-five
gate. Available, nonobsolete types compete with the existing tile best score:

```text
d = clamp(current_energy + provisional, 1, 99)
score = 64*psych
      + trunc((Build+2)*population*economy*64 / d)
      + trunc((Discover+2)*population*labs*64 / d)
```

The two divisions remain separate. Strict improvement preserves a tile tie and
the earlier type on specialist ties. `Insert` accepts that validated winner,
writes at provisional index (not retained plus provisional), and increments
provisional count without changing slots, intake or tile state. Writes stop at
sixteen. A full specialist budget neither inserts nor rescans; otherwise the
remaining vacancy or nutrient deficit determines rescan.

`Fill` starts at retained count, rewrites fill positions below sixteen and
returns total count equal to remaining slots. Its inline selector filters only
prerequisites, keeps earlier ties, starts best score at -999, and evaluates
`3*psych + (economy+labs)` at the configured size, with the parenthesized sum
computed first. Below that size only `3*psych` is evaluated. This is distinct
from the earlier automatic overflow selector and from the later obsolete-type
replacement/output stage. The current port admits a fill only when it has an
available positive winner. All-nonpositive and all-unavailable fallback behavior
is deliberately excluded pending the separate original capture. A path needing
no fill does not evaluate an unused selector.

## Port admission and arithmetic

Population and stored/combined specialist counts are 0..127. Input collection
slots can be 0..128; final specialist count above 127 rejects. Every stored type,
including padding, must be 0..6. Consumption must be positive and the supplied
maximum positive candidate nutrients must be nonnegative. No extra relationship
between population and intermediate counts is invented.

Evaluated signed32 intermediate overflow rejects. Competition preserves the
observed psych, economy and labs instruction order, including denominator
addition before clamping. Final-fill grouping is preserved even when changing
the grouping would make a different intermediate overflow. Unused branches are
not evaluated. This checked policy is narrower than unmeasured extreme original
arithmetic; it does not change collection's separately measured wrapping sums.
All operations stage owned values, so rejection leaves caller inputs unchanged.

## Literal evidence and tests

`tools/reference/project_smac_specialist_allocation.py` fingerprint-checks three
human-owned JSONs and copies their expected values into the native fixture. It
does not calculate expected results or execute the original binary:

- All 17 cleanup cases, SHA-256
  `df13b06445ad7aca682f8add174ca60f0a452ee8b19030853a22b26d4cbc39b5`.
- All 28 competition/insertion cases, SHA-256
  `5b01f88275c599e1d926e10203c74c3cc44d3756adc65a399735f21917f2d076`.
- Seven competition/finalization histories, SHA-256
  `40314a624b6e37fab59274eb7c00db16c6fa18450342976412805eb15a5689fb`.

The last seven original captures ran complete intake. This module's tests use
their declared center-collected, zero-candidate-yield inputs to compose only
competition/insertion/fill, comparing observed counts and stored types. They do
not claim to execute the complete port intake orchestrator. The private sampled
grids are not claimed as port test coverage.

Six native groups additionally cover exact padding/overflow boundaries, malformed counts/types,
grouped arithmetic, strict ties, no-fill behavior and input ownership. On the
integrated source ancestry, seven copied production controls fail intended
assertions: negative-psych removal,
adjacent rechecking, repeated last padding, replacing tile ties, insertion after
retained count, sequential rather than grouped fill addition, and obsolete
filtering during final fill. The checked-out production is never mutated.

The first wrong-grouping control correctly threw on the unexpected intermediate
overflow, but escaped the assertion harness (exit -6). That diagnostic is retained;
the positive-case test now catches unexpected exceptions into an assertion.
Both final-fill controls then fail deterministically with exit 1. This test-only
correction does not change the production arithmetic.

## Focused publication and validation

This publication is based on scoring review `91a4403ae58c095d95bef42f68c03db1358bc905`.
The pure implementation, fixture, projector and six retained test groups match
integrated feature source `b6d356d39452a93c9a139cbcfe1e0dccd72f3c85` exactly.
It omits only the typed resolver declaration/include, its thirteen-line adapter,
two test includes and the resolver test group. No collection or live Base/query
dependency is imported. Registrations are additive on the scoring base.

The actual publication builds in normal Debug and ASan/UBSan configurations.
All six groups pass with allocation tracking in 1.066 seconds normally and
1.116 seconds with ASan/UBSan/LSan, leak detection and halt-on-error. Explicit
LLVM 15 correctness analysis of the specialist and shared worker production
translation units reports zero failures in 0.84 seconds (2.372 seconds including
driver). Rechecking the projector verifies all 17 cleanup, 28 competition and
seven finalization projections against the pinned human-owned evidence.

These are focused Linux publication checks, not a full publication CTest run,
hosted CI, Windows/macOS validation or original binary execution. The integrated
source's seven-group and full-suite receipts belong to its richer ancestry.
Publication logs and the source-equivalence manifest are retained under
`/home/jhurliman/src/glsmac-startup-main-refresh/specialist-publication/`.

The detached orchestrator is a separate consumer of these stages. Live Base
publication still needs a coherent assignment representation and complete
calculation/handler ordering. There are no new world/settings versions, RNG
draws, UI commands or default activation.
