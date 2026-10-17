# Live ordinary land ZOC

The canonical local movement route now evaluates ordinary zone of control
before edge pricing. A move is refused only when both endpoints are controlled;
different foreign Units may control the two endpoints. An authenticated owned
Base endpoint is exempt. Directed Pact exempts a foreign owner, while Treaty
and territorial claims do not. A currently unseen ordinary unconcealed Unit
can control a square in this local route.

Knowledge observers now own optional directed Pact masks separately from their
worker Pact-or-Treaty exemption. Unknown Pact state rejects a foreign interaction;
an explicitly supplied zero means no Pacts. Neither direction is inferred from
the other. Knowledge V3 appends a nine-byte presence/value suffix, checked before
record allocation; its 4 MiB aggregate bound includes the suffix. World26 rejects
older world formats before body loading. Integration preserves the independent
World25 territory prerequisite. A new-campaign diplomacy producer remains
responsible for supplying actual initial no-Pact state.

## Refusal is an executed outcome

Move validation prepares an owned result without changing anything. A ZOC
refusal has no movement-lottery result. Actual execution publishes only the
prepared knowledge: unseen non-Pact foreign neighbors around the destination
gain the observer's visible bit, their square becomes explored, and its current
port improvement fields are remembered. Already-visible neighbors are skipped.
It does not run normal exploration, clear mover visibility, price the edge,
spend movement, change RNG or enqueue a movement presentation update.

Input/domain errors still throw before publication. Unsupported sensor/sight
and foreign-base entry branches remain separate. The current foreign ZOC domain
is singleton ordinary Scout/selected-Rover on flat interior land, with ordinary
roads allowed; sea/air, concealment, mixed stacks, special abilities/plans and
seams are not admitted. The original wrapper's generic attempt bookkeeping and
UI notifications outside the current conventional record are not reconstructed.
No original raw return value is used as a rollback instruction.

## Evidence and checks

The ten native cases project `RE-SMAC-LAND-ZOC-001`'s literal admission,
spending, position and refusal-reveal expectations from
`ordinary-land-zoc-cases.json`, SHA256
`dcd0fd7eae568195a87244e1946d73530f510b4a5deae7c5dcd88fe35a59de9d`.
The fixture retains the actual selected Spartan Rover actor for its Rover row;
its foreign blocker is the already-admitted ordinary Scout rather than an
unrestricted copied foreign Rover prototype. The Base exemption uses an actual
canonical Base with an unstacked mover; it does not import the original extra
friendly stack. Supplied claims test that control ignores ownership; they do
not claim to rerun territory production. These are consumer projections, not
byte-identical original worlds or an original executable rerun.

Actual native/GSE tests also cover reverse-direction Pact, Treaty worker
exemption without ZOC exemption, unknown relations, validation without effects,
split-neighbor refusal revealing exactly one square, saved refusal state restored
through actual Start, and continuous departure/return preserving stationary
visibility. The focused 26 memory-tracked groups pass in 1.066 seconds. LLVM15
analysis of the three changed production translation units reports zero failures
(7.230 seconds including the runner). The first build found a test-macro message
syntax error, corrected before these results. No full suite, platform matrix,
new sanitizer run or mutation campaign is claimed for this follow-up.
