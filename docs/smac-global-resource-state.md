# Owned global resource modifiers

The SMAC simulation now retains atmospheric dust's signed countdown, separate
ordinary-query and upkeep-query solar flags, and an optional Planetary Governor
simulation ID. These are authoritative inputs for the next canonical base
resource consumer. They are not a cached yield or per-base governor setting.

Actual `PrepareNewSimulation` supplies the explicit `NewWorld()` value: dust0,
both solar flags off, and no Planetary Governor. The constructor requires this
state; there is no missing-field fallback. This matches the reset fields in
RE-SMAC-INITIAL-RESOURCE-MODIFIERS-001 and its seven first-worker reset
compositions. The existing no-project campaign capability remains separate;
this change does not claim to store project locations or implement elections.

Restoration supplies every field from the owned record. It does not invoke
new-world initialization, age a countdown, advance solar phases, select a
calculation phase, or infer a council role from base automation. Negative dust
and both solar flags are preserved, including diagnostic states. A present
council ID must be in1..7 and is never remapped through lobby slot order or
faction source order. This storage layer does not impose an unproved election
or elimination policy by requiring that role to name an occupied controller.

The nested Simulation codec is version6. After movement capability it writes
signed32 dust, ordinary solar, upkeep solar, council presence, and the council
ID when present. Older Simulation versions, missing/trailing fields, invalid
Boolean frames, out-of-range integers and invalid council IDs reject before
publication. The existing64KiB nested frame budget remains enforced. Global
settings and the outer world prefix do not change in this slice: both old and
new readers reject an unsupported nested Simulation version before map creation.

## Validation boundary

Tests exercise all four solar combinations, signed dust endpoints and absent/
present council IDs through the complete simulation codec. Actual queued and
immediate Start callbacks observe the fresh values before create-world/start
consumers. Actual world restore, staged publication, retained reconnect and
Early restoration preserve deliberately non-neutral values with the full
simulation and paired RNG. Ten additional malformed suffix cases traverse the
real Client download and exact pre-map validation boundary, preserving the
existing rollback checks.

Independent review corrected one negative fixture to corrupt both Boolean
payload and checksum consistently, so it reaches Boolean value validation
instead of merely failing checksum validation. No production defect was found
in that review. The initial build command failed before compilation because its
Windows-managed worktree required explicit WSL Git environment paths; that
failed log is retained separately.

Full Linux Debug CTest passed 8/8 in 108.56 seconds; the separate ASan/UBSan
build with leak detection and halt-on-error passed 8/8 in 197.79 seconds.
Both builds passed the same focused allocation-tracked simulation, Early
restore and claimed gameplay-RNG groups. LLVM 15 analyzed all 338 Linux
translation units with zero failed analyses in 278.42 seconds; the Windows-only
network implementation is outside that scan.

Four copied-production negative controls failed at their intended assertions:
resetting modifiers on restore, swapping solar phases, removing the council-ID
bound, and changing the fresh dust countdown. The copies were linked into
separate executables and did not replace the tested production binary.
Logs, controls and the file-bound manifest are retained privately under
`glsmac-global-resource-state` in the Linux validation workspace.

No live resource callback, event activation/maintenance, new graphical session,
full-game startup equivalence, or SMAX behavior is claimed by this state change.
