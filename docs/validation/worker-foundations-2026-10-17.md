# Infantry, movement and resource-profile checkpoint

Production revision: `2f3b707bc431310d34c82912dda2ab5bcea5b4cf`.
This follows the [startup/HQ checkpoint](startup-hq-2026-10-17.md) and
[selected Rover HQ integration](rover-hq-2026-10-17.md).

## Integrated changes

| Component | Independently checked source | Integration |
| --- | --- | --- |
| Initialized Infantry presentation | `1e9676f4a083fe21ae99f64491a3e34f2974e390` | `155e53a9` |
| Complete owned resource profile | `9aee9cea6c050aeadcdc45488fff474a1b4269e2` | `074120b7` |
| Gaian ratings and remote HQ records | `6152f5da2e5be851cee726f0332da9e5a4417052` | `924fb325` |
| Destination-fungus admission correction | `015ed8d50dd2a172baeb88ac555d8921d22d1749` | `2f3b707b` |

The [initialized Infantry profile](../cvr-initialized-infantry.md) uses the
actual main software palette and initialized light while preserving Infantry
geometry and scene shades. Its source probe matches all nine retained original
drawings. Existing Scout/Rover profiles remain separately identified. The
actual conventional Unit and retained-preview fixture is included in integration
checks; the Early startup readiness guard survived the merge.

The [resource profile](../smac-resource-rules.md) retains all nine N/M/E rows,
nutrient demand, mine rules and four configured yield-technology tokens. Derived
center/tile views share this one authority. Settings and world readers compare
the complete bounded profile before publication or map allocation. Global
settings are version 10 and world snapshots version 16; simulation 5 and Unit 2
are unchanged.

[HQ movement](../conventional-hq-composition.md) now composes real Gaian ratings
and remote same/foreign-owner records, including reordered/tied records. Local
authority, endpoint ownership, occupancy and source-stack restrictions remain.
World continuation compares complete semantic state: only the two named
unordered definition tables are sorted for comparison; all their values and
the remaining body bytes are retained.

The [fungus correction](../conventional-fungus-admission.md) rejects destination
fungus without an admitted endpoint HQ before RNG or authority changes. Earlier
empty-world evidence supplied zero nearest-base distance and did not establish
the separate native-life encounter path. Source-only fungus exits remain
admitted; the positive lottery/upkeep test now uses a supported HQ context.

## Combined validation

| Environment | Complete CTest result | Test time |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 | 106.88 seconds |
| Linux ASan/UBSan/LSan | 8/8 | 188.44 seconds |
| honey native arm64 Debug | 8/8 | 69.65 seconds |

Linux enables leak detection and halt-on-error, using the established non-PIE
sanitizer build. All 73 focused native groups also pass with allocation tracking
on Windows and Linux. The groups cover Early/conventional/selected-Rover paths,
CVR descriptors/rendering/ownership, HQ/controller continuation, resource
resolution and full-profile persistence. The initialized Infantry Unit/preview
fixture uses the purchased installed assets in these focused runs, with Null
graphics. The Windows executable was run through a waited subprocess with its
exit status and success output checked.

Full LLVM 15 analysis of 338 production translation units reports zero failures
in 265.41 seconds. The Windows-only network implementation is excluded from this
Linux scan. Header changes warranted the full scan here; the small movement
source changes separately used one-TU analysis and focused tests during iteration.

honey used a fresh clean checkout at the exact production revision, from
14:01:18 to 14:03:27 UTC. Its binary SHA256 is
`3a43d77489efe9229a8f7a6bf0437db918e4f41a5bfe0eec568e76570c2e926d`.
This is native compilation/CTest evidence. The last human interactive map/base/
turn/new-game/close smoke test remains revision `5cd78418`.

The private integration manifest is
`out/worker-foundations-integration-manifest.json`, SHA256
`a82cf2b32c1b802582317c39c6504bf08778e3953959969fcc2051f9df26e1fd`.
It binds the source, component receipts, actual group names, binary hashes,
Windows/Linux logs, analyzer summary and copied honey logs. Original assets
and the researcher's uncommitted evidence are not part of these commits.

## Next consumers and release limits

Default starting-unit definitions and live resource callbacks remain unchanged.
The next consumers are conventional UI stats, owned fungal capabilities,
specialist definitions and canonical base resource/worker calculations. Accepted
original first-worker/reset compositions now distinguish persistent worker
choices, base options, global council status, local effects and derived yields.
Ordinary movement away from HQ has new original evidence awaiting implementation;
distant native encounters, broad terrain, combat and full SMAX gameplay remain
separate work. This checkpoint is not a complete playable strategy match.

The renderer policy prerequisite [PR #158](https://github.com/jhurliman/glsmac/pull/158)
has nine successful CI jobs and three skipped jobs at
`9286bac1515f99f72e991f960ba51488be58f401`. Current draft heads remain unmerged
pending a completed GitHub Codex review after the review quota is available.
Local integration checks and earlier reviews do not constitute that review.
