# Existing worked-square collection

`SmacWorkedSquareCollection` implements a detached existing-allocation stage.
It accepts a selected-worker mask, a resolved unavailable mask, population,
preexisting N/M/E intake and sea marker, and earlier retained/provisional
specialist counts. It returns cleaned selection, the shared worker-stage state,
and an explicit next boundary. It does not mutate a Base or activate commands.

## State and ordering

Cleanup removes unavailable selected bits except the existing center bit. It
does not turn on an initially absent center bit. Admission counts every one of
the 32 cleaned selected bits, including indices outside the 21-square radius.
If that count exceeds population plus one, the result is `Repair`, with no yield
query, no visited bits, and unchanged seeded intake/sea marker. Repair itself is
not implemented.

Admitted traversal visits center index zero once regardless of its selection
bit, followed by selected indices 1..20 in ascending order. Each visited bit is
set before its query. The query returns one optional, owned whole-tile N/M/E
triple and sea flag. A missing result represents a clipped square: it contributes
nothing but still consumes one collection slot and remains visited. A valid sea
result sets the persistent participation flag. Traversal stops immediately when
slots reach zero; otherwise it ends after index 20, retaining any unused slots.

The callback is a supplied-yield abstraction, not an emulation of original
per-resource dispatch. It receives the staged state before this tile's resource
addition and before applying its returned sea flag. It must not publish live
state. Original resource helper side effects and the original sea flag's timing
within those helpers are outside this interface. A thrown query abandons the
owned result and leaves input unchanged; external callback side effects cannot
be rolled back by this value operation.

Every intake addition has defined signed32 wrapping. The measured collection
path has no aggregate saturation or final zero clamp. This is distinct from the
allocator scorer's admitted checked-arithmetic domain; the collection does not
silently broaden another stage's contract.

After collection, equality between remaining slots and the supplied retained
plus provisional specialist count selects `FinalFill`. Otherwise the result is
`LaterAllocation`. There is no food-sufficiency test at this early gate. In
particular, an already complete all-specialist allocation can retain a nutrient
shortfall, and a complete valid worker allocation is not automatically optimized
against a better alternative. These are next-stage decisions, not complete
specialist finalization or complete intake results.

## Coordinates and admitted domain

The optional `Resolve` method uses the measured 21 doubled-X radius offsets,
one horizontal width adjustment or clipping, and unconditional vertical clipping. It takes
explicit dimensions and center coordinates; it does not read a live map. The
port admits positive dimensions, an even width at least two, an in-map center
with matching coordinate parity, and radius index 0..20. Widened intermediates
avoid signed32 coordinate overflow. These validation rules are an explicit port
domain, not a statement that every admitted dimension is an original map preset.
The original named coordinate cases use width 128 and height 64.

A follow-up audit of the collection's own pinned instructions at
`4fd8c3..4fd8d4`, followed by bounds checks at `4fd8fd..4fd907`, corrected
the initial modular wrap to a single adjustment. On width two, a radius offset
of three can remain outside the map and must clip. Four literal narrow-map
assertions distinguish clipped and valid one-adjustment cases; they are static
instruction-derived tests, not additional original execution. All five collection
groups pass with tracking after this correction. A copied original-modulo TU
fails the narrow-map assertion (exit 1). Existing 128-by-64 projections are
unchanged. The focused build also includes the separate, uncommitted specialist
bookkeeping work; no new whole-suite claim is made for this correction alone.

Population and specialist counts are admitted in 0..127, with combined
specialist budget at most 127. No relation between population and those counts
is inferred; malformed/intermediate masks remain observable at the measured
admission boundary. The shared `SmacWorkerStepState`, `SmacWorkerTileYield` and
`SmacWorkerYieldQuery` types are reused from `SmacWorkerAllocationRules`.

## Evidence and tests

`tools/reference/project_smac_worked_collection.py` fingerprint-checks the
human-owned inputs and projects literal outputs into an owned fixture:

- All 19 named `RE-SMAC-WORKED-COLLECT-001` cases, SHA-256
  `0ea35cb1492dcf611cc88e0c5689a3a699053d12bbf7a1074db8021ac3eb29da`.
  The 500 private sampled cases are not copied or claimed as port coverage.
- The 168 collection rows from the 504-row `RE-SMAC-BASE-UPDATE-001` fixture,
  SHA-256 `7764998ce1f9da001783a8bb202262a8a1d83e46fbc3bb0f7ee2664beefd4ff3`.
  Their actual returned tile yields are supplied to isolate collection arithmetic.
  Direct incremental add/remove rows and six continuous sequences are excluded.
- Three early-completion rows (002, 007, 009) from the ten complete-intake
  `RE-SMAC-WORKER-INTEGRATE-001` cases, SHA-256
  `823718431f75ccddd028cf4fd29b3c17de86a76f0a5f0bf03bfff606d925febc`.
  Only their existing collection totals/visits and early boundary are checked.
  The full original intake pipeline is not reimplemented by these tests.

Five dedicated native groups also cover staged callback order, clipped-slot
consumption, preserved seeded sea state, callback exceptions, all32-bit cleanup,
upper population/count bounds, provisional-budget equality and widened coordinate
resolution. No original executable, installed assets or human research file is
modified or executed by the projection or native tests.

## Remaining integration

The unavailable-square producer, existing-specialist split/compaction,
managed/upkeep bypass, repair, scoring/assignment loop, final specialist fill,
ecology and complete resource/citizen recalculation remain separate stages.
Neither this value module nor its callback invents exploration/territory,
faction ownership, governor state or a universal starting population. Existing
Base/Pop allocation and legacy APIs are unchanged.

## Validation

Source ancestry preserves specialist queries `e5c54fdd` and cherry-picks exact
allocator source `da31a3ff006ba2a6275d185d8aee7df239783e6f` as `b8d6af10`.
Only additive source/test registrations merged; the shared types were not copied
or changed by collection. Final local Linux validation:

- Full normal CTest: 8/8 in 90.40 seconds.
- Full ASan/UBSan/LSan CTest, leak detection and halt-on-error: 8/8 in
  198.15 seconds.
- Five focused groups passed with allocation tracking in both binaries.
- Full production correctness analysis: 344 translation units, zero failures
  in 140.39 seconds (144.12 including the driver).
- Seven copied-production controls fail intended assertions: forced center bit,
  ignored high selected bits, free clipped slots, extra food gate, lost sea
  participation, wrong radius offset and saturation instead of signed32 wrap.
- The projection's fingerprint and exact-output check passes for all three
  pinned input files.

The initial overflow control threw instead of returning a wrong value and
aborted beyond the test harness. That diagnostic remains retained; the final
control uses saturation and fails the intended numeric assertion. Only copied
TUs are changed for controls; the intended source remains unchanged. Logs,
copied variants and the final source/receipt manifest are retained under
`/home/jhurliman/src/glsmac-startup-main-refresh/` with `worked-collection`
names. These are local Linux checks, not Windows/macOS or hosted CI receipts.
