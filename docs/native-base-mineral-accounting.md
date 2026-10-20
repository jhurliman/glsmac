# Complete mineral accounting and ecology stores

`AccountSmacBaseMinerals` implements original `0x4ff1e0` over staged Base records.
The selected Base supplies owner, population, raw intake, terraforming and target;
the separately indexed Base supplies facilities. Incoming minerals enter first,
combined half-bonuses scale once, and support plus outgoing minerals form demand.
The pass preserves mineral stock and all faction records; it does not allocate
workers, advance production or run ecological events.

The ordered writes retain the original intermediate surplus, zero waste and
pre-orbital surplus. Ecology uses processed minerals before support and the late
Space Elevator bonus. Nano/Singularity overlap contributes one ecology divisor;
project ownership is resolved through host Bases. Voice of Planet existence is a
separate global condition. The Elevator doubles intake only for orbital targets
-34 through -37, after ecology, preserving the pre-orbital value.

Production amounts and ecology share their arithmetic core with existing live
resource callers. Those callers retain checked narrowing and input validation;
the raw-record boundary uses defined DWORD wrapping at original instruction
boundaries, avoiding C++ signed overflow. Matching this component does not yet
connect generated worker intake, energy, citizens or the live new-game caller.

## Evidence and validation

Frozen bundle `independent-startup-mineral-accounting-evidence-20261020` contains
217 verified files; manifest SHA256 is
`30f3bc6a615257a3754cb19b189158df68f65cd3ec6296e2a85107d68ecc7d4b`.
Original executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The compact numeric fixture reconstructs independent inputs and checks complete
seven-Base/eight-faction buffers and 2,761 ordered stores across 212 controls.
Controls distinguish facility subsets, owned/foreign projects, overlapping
Singularity effects, human/AI scaling, orbital targets and separate Base contexts.
Purchased assets and full original buffers stay outside Git. There is no RNG input.

Cached incremental build and 30 selected native groups pass, including nutrient,
opening support, existing canonical resource intake and ecology regressions.
Correctness analysis passes for four affected production translation units.
A focused six-TU ASan/UBSan build passes the same 30 groups; other objects are
uninstrumented and leak detection is disabled. Platform matrices were not repeated
for this accounting component. This change has no new runtime visual output.
