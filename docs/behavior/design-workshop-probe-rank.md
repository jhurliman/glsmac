# Complete scratch Probe rank and direct-MORALE mapping

2026-10-21. Original executable SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This closes the plan11 branch in the
[scratch-rank/lifecycle contract](design-workshop-cached-rank-and-lifecycle.md).
[Twenty-five complete original controls](design-workshop-probe-rank-cases.json)
execute5d9ca0/5d8420 initialization,5d8f50, actual5d17b0 technology admission,
and56fd50 row12 variant/address selection. Only sprite painting is supplied.
Installed tech metadata and owned/project/faction states are declared inputs;
no native expected rank is an algorithm input.

Private additive freeze:
`tmp/reference/independent-workshop-probe-rank-evidence-20261021/`.
Manifest SHA-256:
`3dcd5749a353c94151bf9a82b2733effa158feb4040aa80d013359bc621ca971`.
There are16 payloads. All25 controls pass. Seven installed faction bonus-list
audits and relevant current parser metadata are also retained.

## Full positive-owner Probe-plan formula

5d8f50 first branches on the **scratch design's plan byte42==11**. This is
before its predefined-Psi or conventional-MORALE paths. For owner1..7:

```text
S = unsigned stored Unit morale byte36
P = clamp(retainedSocialPROBE, 0, 3)
J = 2 if the Telepathic Matrix's host Base belongs to owner, otherwise 0
T = number of technologies with flag bit0x2 for which 5d17b0(techID,owner) succeeds
effectiveRank = clamp(S + P + J + T, 2, 6)
row12Variant = effectiveRank
```

Retained PROBE is95d440+owner*20bc, corresponding to social axis8; this is
not retained MORALE at95d430 or the current social vector. Source addresses:
5d8fb6..5d8fd1 clampsP;5d8fd3..5d9004 addsJ;5d9006..5d902a scans techs;
5d902c..5d9057 addsS and clamps2..6.

For the Workshop constructor, `S=uint8(directFactionMORALE+1)` as already
proved. Probe rank does not add the direct scalar a second time and does not
apply conventional retained MORALE, direct negative-morale immunity, home
Creche/riot or normal production training. Direct MORALE affects this path
through initialized storedS only. Neutral inputs therefore yieldS1, rank2.
Direct+2 yieldsS3, rank3; direct-2 wraps toS255 and yields rank6.
The query's mode/third-argument values do not add a Probe-only modifier here.
Owner0 is dispatched to515d50 before this plan test and remains a separate rule.

## Project input is Telepathic Matrix69

The project location DWORD is95a094. The project-location array begins95a01c
for one-based facility39, so this is `39+(95a094-95a01c)/4 = 69`.
The installed facility69 is **The Telepathic Matrix**.

If the location is negative, add0. Otherwise compare the host Base's unsigned
owner byte at96f1b4+304*hostID with the querying owner; equality adds2.
It does not require the scratch Unit to have a home Base, or the preview's
current Base to host the project. It does not check a local facility bit.
The ordinary native input domain has a valid host Base ID when nonnegative;
no new bounds validation is claimed for corrupt locations. Native controls
distinguish own host(+2), foreign host(0),-1(0) and-2(0).

## Technology input and admission

The loop visits89 definition rows, flags byte at9b7ba8+44*ID, testing0x2.
Count each owned admitted flagged definition once. Do not count all owned
technologies, match display names, or apply a universal fixed tech bonus.
The installed flagged rows are:

| Zero-based source ID | Key | Name |
| --- | --- | --- |
|22|Poly|Polymorphic Software|
|30|Algor|Pre-Sentient Algorithms|
|33|DigSent|Digital Sentience|
|34|HAL9000|Self-Aware Machines|
|41|MindMac|Mind/Machine Interface|

The actual5d17b0 predicate, for these nonnegative loop IDs, requires:

* A positive querying owner and ID<88; definition88 is rejected even if its
  supplied flags and ownership byte are positive.
* Signed first prerequisite reference>=-1.
* Signed second prerequisite reference>=-1 **or** first reference==-1.
* The owner's bit set in ownership byte95a0fc+ID.

None is-1 and Disable is-2. This checks definition admission and the ownership
bit; it does not re-research/revalidate the prerequisites' current ownership.
An unusual firstNone/secondDisable definition passes that sign admission;
firstDisable fails, and first ordinary/secondDisable fails. All three are
separately distinguished by new native controls. The helper's special
ID-1 behavior is irrelevant to this loop, which supplies0..88.

The port already retains `SmacTechnologyRule.flags`, two prerequisite tokens,
canonical source IDs and owned technology state. Resolve those metadata into
the same counted-definition predicate before incrementingT. Five isolated
flagged-tech controls useS2 so a single added point is visible above the rank2
floor. Unflagged owned techs and foreign-only ownership do not contribute.

## Direct MORALE: parser gap versus admitted stock values

At inspection time `SmacFactionStartupBonus::Kind` ends at FUNGENERGY and has
no direct-MORALE tag. `Parse` supports SOCIAL/IMMUNITY axisMORALE, but its
numeric-keyword list omits **MORALE**, so a direct pair is rejected rather than
retained. Existing SOCIAL MORALE entries cannot supply the direct scalar or
its unit-immunity flag.

All seven actual installed bonus lists—gaians,hive,univ,morgan,spartans,
believe,peace—contain **no direct MORALE pair**. The new cases retain each
file hash and parsed bonus pairs. Accordingly, for those admitted stock
profiles, direct scalar0 and direct unit-immunity=false are source-backed
values. Spartan SOCIAL++MORALE and Gaian SOCIAL-MORALE still affect retained
social ratings; neither is a direct numeric MORALE pair. A preference line
mentioning MORALE is also not a bonus pair.

For profiles extended to support direct MORALE, reuse the already native-checked
[direct parser contract](direct-faction-morale.md):

1. Definition initialization sets scalar0 and clears direct unit-immunity.
2. Each case-insensitively matched direct MORALE entry replaces the signed
   scalar with its parsed numeric value; repeated entries do not sum.
3. Every zero-valued direct entry sets unit-immunity. Later nonzero entries
   leave that flag set. Thus scalar is last-entry-wins while immunity is sticky
   until definition reinitialization; final scalar alone cannot recover it.
4. Direct MORALE consumes no shared eight-record slot and remains effective
   after the record budget fills. Preserve it separately from the admitted
   record projection. Do not model it as a SOCIAL/IMMUNITY record.
5. Narrow to the low byte only at scratch Unit initialization. Keep the signed
   scalar and sticky flag for other morale consumers; the ordinary conventional
   query uses them again, while this Probe query does not.

This packet documents the current ingestion gap and exact source mapping;
it does not edit the parser, codec or production adapter. The earlier40 parser
controls/120 queries remain the direct-MORALE parser proof and were not rerun.
