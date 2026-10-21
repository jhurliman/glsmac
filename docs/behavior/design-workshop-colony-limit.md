# Colony production at the 2048-Unit limit

**Allocation refusal precedes Colony population loss and Base removal.** A
size-one human can accept abandonment, lose the production minerals, and
still retain its Base when the allocator then refuses the new Pod. The
size-one choice happens before allocation; do not replace it with an early
unit-cap rejection.

Authority: GOG `terran.exe`, SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The [four full-return cases](design-workshop-colony-limit-cases.json) extend
[ordinary Colony production](colony-production-loop.md) and the
[manufactured Unit refusal tail](design-workshop-newborn-state.md#unit-count-2048-refusal-tail).
They use the original predefined Colony design zero, human owner one, a
pending Scout design two, and a supplied sparse global Unit count of 2048.

## Actual order

1. The ordinary resource pass runs. Production posts mineral surplus and
   checks the admitted cost before its Colony size-one policy.
2. `505e5d..505e7f` checks Colony mode eight, population exactly one, then
   stored owner difficulty. Population two or greater bypasses the size-one
   policy. Stored difficulty at most one also bypasses it.
3. In the admitted hard human case, with production governor inactive and
   the existing special-mode deferrals absent, the original question wrapper
   `499b70` returns to `5061b4`. Choice zero delays. Choice two accepts
   abandonment and reaches `506275`; it does not remove the Base there.
4. The normal production debit runs at `506311..506368`, including mineral
   baseline update. The actual allocator `5d8520` is called at `50638b` while
   population and Base existence are unchanged.
5. At count 2048, `5d8520` returns −1 before allocating/initializing a Unit.
   `506397` takes the failure cleanup tail and returns from production.
6. The Colony population/removal code is later, on the **successful**
   allocation branch: population decrement at `5066d2..5066d7`, zero-population
   test at `5066df..5066e4`, newborn-home clearing at `5066f3`, and Base removal
   helper `4faad0` called at `5066fa`. Refusal never reaches any of them.
7. The outer `50c760` Base aftermath still runs population/food processing,
   refreshes baselines and executes the later resource pass. The failed build
   does not advance the current/pending production queue.

The allocator failure also precedes the later successful-Colony easy-difficulty
and protection checks at `5066a7..5066d0`. The new easy control distinguishes
the earlier question bypass; it does not need a population refund or a Base
restoration operation after refusal.

## Complete native results

All four contexts return normally from `50c760(0)` with return value zero.
Each cost is 30. Two declared forest squares give mineral surplus five at
size two and three at size one. Stored minerals start at 25 or 27, so the
actual posting reaches 30 before the policy/debit.

| Input | Question | Allocator | Minerals / baseline after | Population after | Base after |
| --- | --- | --- | ---: | ---: | --- |
| Size two, stored difficulty three | None | Refused | 0 / 0 | 2 | Preserved |
| Size one, stored difficulty one; global difficulty three | Bypassed | Refused | 0 / 0 | 1 | Preserved |
| Size one, stored difficulty three, accept abandonment | Original wrapper, supplied answer two | Refused | 0 / 0 | 1 | Preserved |
| Size one, stored difficulty three, delay | Original wrapper, supplied answer zero | Not called | 30 / 30 | 1 | Preserved |

Acceptance and Delay see 30 minerals before and after the question wrapper.
Acceptance reaches the allocator after the debit, with population still one.
This is a failed allocation after acceptance, not a cancellation or a protected
successful Pod completion. The allocator is called once with arguments
`(design=0,owner=1,x=16,y=16)` in each non-delay case.

In every case Base count stays one and Unit count stays 2048. All 2048 Unit
records and all Design records remain byte-identical. No newborn, population
decrement, Base-removal call or success queue-advance call occurs. Current
target and target baseline remain Colony zero; pending count remains one
and its next entry remains Scout two. Size-one food increases from zero to
one in the outer aftermath; size-two food remains zero. The final mineral
surpluses remain three and five respectively and are not posted a second time.

These four cases use a built-in design, so they do not newly exercise custom
prototype mutation. For a custom Colony recipe that reaches `506275`, the
previous source-pinned prototype/history branch still runs before debit and
allocation. The existing custom Infantry refusal controls establish preservation
of those mutations; no additional custom-Colony prototype matrix is claimed.

## Scope and evidence

Private additive bundle:
`tmp/reference/independent-workshop-colony-limit-evidence-20261021`.
Manifest SHA256:
`8e0d3ed34da34c3ecb850ffa050e90300735986bea63ec881e314d2e08057d41`.
Its 11 payloads contain complete native input/output buffers, ordered writes,
events, independent exact-state checks and source disassembly. The four
complete returns comprise three allocator refusals and one preallocation
deferral. Population two represents the source's positive population-greater-
than-one branch; no separate population-three or easy-size-two run is needed
to distinguish that straight-line admission path.

The original question wrapper executes; its UI constructor/template/bitmap,
modal choice and cleanup services are explicitly supplied. Modal answer two
is declared user acceptance, not inferred approval. This is controlled engine
execution without an interactive executable launch. The inherited headless
movement/visibility and listed presentation/destructor boundaries remain.
The count is a supplied sparse-limit fixture, not a naturally populated
2048-Unit game. General AI/governor/special-mode choices, protection variants,
arbitrary food thresholds and successful last-Base disposition remain in their
separate contracts. No production code or checkout references are changed.
