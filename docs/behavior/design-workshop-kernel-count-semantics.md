# Native Workshop kernel count: requested versus processed records

2026-10-21. [Scalar audit and native controls](design-workshop-kernel-count-semantics-cases.json).
Pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

**Corrected chassis expectation: processed_records=42810.** The earlier42874
is the inclusive **kernel_count_operand_sum**, not the executed loop count.
All raw color/depth bytes and hashes remain unchanged. The old frozen packet
is preserved; current public count definitions are corrected.

For each observed6867f0 call, argument4 is the requested count and argument6
is flags21. At686bf8, native code loads the count into ESP as a temporary
loop counter.686bfc tests bit20; **686c03 decrements once when it is set**.
Bit1 selects the single-sample branch. Its actual body begins686cdc and
686d49/686d4a decrement/loop. Thus flags21 execute exactly `requestedCount−1`
records. The final terminal record is not visited. This is a flag-based count
adjustment, not scanning each record until code26 is encountered.

The original64 call operands sum to42874. Every call has flags21, so there
are64 excluded terminals and **42810 actual record-body iterations**.
There remain64 kernel/group draws and58 geometry-function entries.

The audit executes the original patched kernel with self-authored zero-step
streams. Two three-record controls execute3 iterations with flags1 and2 with
flags21. Another64 controls use **each actual captured count operand**, with
flags21, and count actual visits to686cdc. They total42810. The three-record
stream has a final code26 record, establishing that flags1 visits it whereas
flags21 excludes it. No port implementation or port counter is an input.

This conclusion is scoped to the observed6867f0 flags21 count-based branch.
Bit10 selects a separate header-driven path; no general rule for that path or
unobserved clipped-kernel variants is claimed here. The original terminal
contract used by the record decoder/rasterizer remains intact.
