# Frozen SMAC faction startup metadata

Host preparation now freezes the owned faction startup profile with the numeric,
production, technology and world-size profiles. It reads `alpha.txt` once, checks
the canonical faction bindings, and reads their seven faction files through the
same existing ResourceManager-backed source callback. Every profile is staged
until the last file parses successfully. Missing or unsupported faction data
fails preparation without publishing any profile or changing unrelated settings.
The settings selection remains retryable; this does not change ResourceManager's
existing path caching or promise hot reload of a newly installed missing file.

The [parser contract](smac-faction-startup-profile.md) defines accepted keywords,
text policy and source identities. In particular, the profile keeps ordered raw
bonus pairs, duplicates, supported records beyond the shared eight-record budget,
and non-record numeric metadata. Named technology references use the frozen
technology catalog, including disabled definitions and position 88. They are not
filtered by ordinary research or production availability. Numeric `TECH` remains
an extra-count field, and `None`/`Disable` remain distinct sentinel references.
No technology possession, social rating, energy, facility, population or other
startup effect is applied by preparation or transport.

Joining clients read no local rules or faction files. They receive the complete
host profile before the existing state-ready callback. SMAX has no SMAC faction
startup profile and reads none of these SMAC files. Reset and new ruleset
selection discard the previous profile; already-frozen host preparation does
not read it again. Unknown/custom faction source mappings remain unavailable
under this explicitly bounded profile.

GlobalSettings version **6** appends the existing faction-profile version-1
payload after the world-size profile and before MapSettings. Decoding uses the
incoming staged technology catalog to validate every raw/derived pair. Locked
settings require exact equality of all source bindings, ordered fields, discarded
records and scalar metadata. Complete global framing and all remaining fields
are checked before publication. Invalid late faction records preserve existing
map settings, full local filename, other profiles, difficulty, name and lifecycle
locks. This is a global-settings transaction, not a transaction for a malformed
later FactionManager body in State.

Older GlobalSettings versions reject. This commit does not extend the world
snapshot header: matching the faction profile at that boundary is the next
separate change. No stable save compatibility or live named-tech initialization
is claimed.

## Validation

Synthetic tests exercise the actual host source callback, GlobalSettings codec,
State/Packet transport and Client readiness/rejection path. They check eight
ordered reads, missing/malformed seventh-file failure, SMAX/join read absence,
reset, duplicate named references and independent numeric counts. Frozen-equality
tests vary every bonus position in every faction, including a discarded ninth
record, raw spelling with unchanged meaning, order, keyword and scalar metadata.
Malformed late source identities and derived fields are also checked on locked
and unlocked destinations. Existing valid technology-difference fixtures now
retarget their synthetic named token when a catalog key is renamed, so they
continue to exercise a coherent incoming profile rather than failing early on an
unrelated missing token.

Source ancestry is world-size `fe87bb2ba788d5a9c29b23b08a43d67e8de80c24`, the
pure faction parser `f99b43fd6b1ceddf86c37df7a72d86dfcd2be232`, and the positional
RESOURCEINFO correction `a14ae649609bac8cf568d012263d5741e64e6aab`. No research
files or original assets are part of this change.

On the local Linux/LLVM 15.0.7 toolchain, the complete normal CTest suite passed
8/8 in 63.64 seconds. Eight focused cases passed with allocation tracking, then
with ASan, UBSan, LSan and allocation tracking enabled; both runs reported no
tracked leaks. The sanitizer executable used the already-established local
non-PIE workaround. Focused analysis of Settings.cpp passed with zero findings
in 1.58 seconds. Because Settings.h changed, the full production analysis gate
remains required; the combined follow-up world-header source will receive that
scan rather than treating this focused check as equivalent.

Two copied-production negative controls failed their intended assertions:
omitting faction equality accepted a frozen-field change, and prematurely
publishing the base-square profile exposed partial state after a late faction
parse failure. The intended production sources were not mutated. A local
installed-data probe linked those unchanged production objects with a copied
test registration and read the actual eight GOG files. It verified the seven
named technology references, the University's separate extra count, complete
settings roundtrip equality, and absence of a second read after freezing. These
are backend validation results, not a rendered-game or live startup-effects
claim.
