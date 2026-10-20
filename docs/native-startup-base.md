# Initial Base record and radius preparation

`SeedSmacStartupBase` implements the original turn-zero constructor through
`0x4faa7b`, before radius, territory and resource/citizen passes. It retains a
complete 304-byte Base record, mutating only original-owned fields, plus the
owning faction's Base/presence counts and the founding tile. Names and cached
catalog costs are explicit owned inputs; automatic name-list cursors and
automatic design production are separate stages.

## Allocation, record and economic state

At 512 Bases the constructor returns refusal without changing storage. Otherwise
it sets the founding tile's Base bit and affiliation, preserves its cached rating,
updates the positive owner's remembered items, increments owner/region counts,
and returns the original eight-byte founding-history entry for the caller to
append. Disclosure uses existing exploration plus the owner bit; this does not
itself explore the tile. Population-memory bytes follow that disclosure.

The record receives position, current/former owner, supplied name and terminator,
governor preferences, sentinels and original zeroed resource/storage fields while
preserving untouched bytes and name padding. The turn-zero production override
selects Formers when the canonical prerequisite query succeeds, otherwise Scout
or Colony Pod according to stored owner difficulty. Initial mineral credit is
the truncated half of mineral factor times cached design cost. Both switching
baselines retain the selected item and credit before later resource calculations.

The constructor reuses `CalculateMineralCostFactor`. Turn zero disables the late
dominance subtraction; global difficulty, human/reference-owner masks, network
state, multiplier, map-size scaling and current INDUSTRY remain distinct inputs.
The original discontinuity outside INDUSTRY -7..4 is retained. No duplicate
mineral-factor formula or newly inferred clamp is introduced.

First-Base HQ, local/network population, network facilities and morale, sea
Pressure Dome, clamped FACILITY bonuses and the legacy free-facility technology
table follow source rules. Positive current GROWTH can raise first-network
population. The caller resolves the two definition penalty flags into the
population-penalty input. The stock loader disables all legacy Free-tech fields;
explicit enabled-table controls still exercise the ownership query.

## Radius and goal cleanup

`PrepareSmacStartupBaseRadius` visits 49 original offsets. Inner 21 mark radius
membership and its owner bits; outer 28 clear rating nibbles without changing
affiliation. Vertical edges clip, and round worlds wrap X once.

The subsequent all-faction goal cleanup uses the final attempted coordinate,
Base plus (-6,0), including a clipped coordinate. Persistent type-8 goals within
distance 3 lose type and priority; matching active goals lose type while keeping
priority and other fields. The helper returns this actual cleanup coordinate and
can report ordered tile stores with original instruction PCs. It does not replace
the subsequent territory, resource, citizen or aggregate producers.

## Original evidence and validation

Frozen bundle `independent-startup-base-seed-evidence-20261020` has manifest
SHA256 `f8245b6eeabcbbcf48415e96da5f33c38f380a4346690e8223dfb312f6ff9b05`.
All 169 files were verified before importing compact numeric fixtures.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Full captures and purchased assets remain outside Git.

- 83 constructor controls compare all Base, faction and tile bytes, allocation
  result/count and founding history from poisoned buffers. They cover size and
  mineral scaling, all computer difficulties, INDUSTRY -8..6, reference-owner/
  network branches, first/non-first, population/facility branches, allocation
  limits and region-count byte wrapping. Final records are checked; the transient
  production scan's intermediate stores are not reproduced or claimed.
- 16 radius controls compare complete tile and eight-faction records plus 752
  ordered tile stores including PCs. Goals distinguish Base-center cleanup from
  the actual final-offset behavior, preserving active priorities and other fields.
- The generated first-site test continues from zero storage through the named
  award into Base/radius preparation using owned name/catalog inputs. All 304
  Base bytes and 140,800 tile bytes match original boundaries `4faa7b` and
  `4faa83`. This is a Base/tile projection: unclosed automatic-design/capability
  writes to intervening faction state are not supplied or claimed equivalent.
  Cleanup is (19,35), with 70 tile stores and unchanged CRT 2011860586/shared 110.

The cached incremental build and nine selected native groups pass, including
existing mineral-factor controls. Two production translation units pass
correctness analysis. A focused four-TU ASan/UBSan build passes the same groups,
with other objects uninstrumented and leak detection disabled. No platform matrix
was repeated.

Full founding still requires the following territory/cache and resource/citizen
passes, automatic design/capability and naming state, factory publication, and
complete controller composition. See the
[generated-world integration plan](native-generated-world-integration.md).
