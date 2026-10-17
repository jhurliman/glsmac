# Selected conventional Scout and Spartan Rover presentation

The existing [conventional Unit](conventional-unit-state.md) can now instantiate
the admitted Spartan startup Speeder with the explicit
[Spartan Rover descriptor](cvr-rover-unit-adapter.md). The Infantry Scout path
retains its existing admission. This adds no default startup spawn, movement,
upkeep, new renderer, or Early-phase stat approximation.

The current or private downloaded bound simulation resolves the participant,
owned design and Refreshed social state before Unit construction. A Speeder
additionally requires the canonical `SPARTANS` faction ID, an actual owned
generated design, and equality with that owner's saved `starting_scout`.
No design ID is inferred from a lobby slot. The existing component gate still
requires Gun/Scout armor, reactor 1, no abilities or faction exclusion, and
Recon plan 3. The frozen faction profile remains fully validated.

The resolved owned context carries a checked presentation value. UnitManager
requires an exact pair: Infantry/Scout context with `RT_CVR_SCOUT` and immutable
Scout composition, or selected Spartan Speeder/Rover context with
`RT_CVR_SPARTAN_ROVER` and immutable SpartanRover composition. The latter
descriptor uses fixed Spartan source material, so another faction's otherwise
valid Speeder cannot borrow it. Predefined Rovers and nonselected custom designs
are outside this bounded producer. Restore applies the same checks using the
decoded saved context, not the destination's previous selection.

Facing, camera inputs, render morale mode and placement remain explicitly
supplied, validated values. Fixture facing 2 is explicit; this patch does not
choose a default renderer policy. The original allocation evidence records
facing 2, but reproducing the complete startup presentation remains separate.
Canonical owner, Unit ID, tile membership, tagged bytes and serialization formats
are unchanged. There are no new fields in the saved Unit or simulation schema.

## Tests and scope

Two dedicated groups drive the real GSE spawn method and actual world-body
restore. They verify selected Spartan identity, mode-0 effective morale 2,
full allowance 6 at Q3, copied spawn fields and exact saved-state round trips.
The destination deliberately has a different valid current selection, so only
the private downloaded selection can authorize the restored Rover.

Rejection cases use structurally valid synthetic owned rows to distinguish
wrong faction, nonselected design, predefined Rover, foreign owner block and
crossed render descriptors. They assert unchanged ID allocator, units, tile
membership, spawn hooks, frontend updates and whole saved world, including both
gameplay RNG words. These supplied rows are not claims about what the startup
design generator normally produces.

The existing real Rover frontend/widget fixture now creates an actual
conventional backend Rover and dispatches its real `FR_UNIT_UPDATE` through
frontend Game. It checks fresh movement/morale, exact actionability, distinct
composition pixels and retained widget texture ownership through teardown.
Synthetic CVR/material/badge files keep the ordinary test asset-free; this is
not a new installed-asset or original-renderer comparison.

The source is based on integration `d6afde1a`. Linux Clang 15 normal CTest passed
8/8 in 81.79 s; ASan/UBSan/LSan CTest passed 8/8 in 164.11 s with leak detection
enabled and the existing local `-no-pie` sanitizer startup workaround. Ten named
groups (the two new groups, six prior Unit groups and both real previews) also
passed with memory tracking in each build. The initial focused filter misspelled
the Rover registration and ran nine groups; its log is retained alongside the
corrected ten-group runs, without treating the initial run as preview coverage.

Focused correctness analysis of GameSimulation.cpp and UnitManager.cpp reported
zero failures. This two-TU check does not replace full analysis of shared-header
dependents in the combined checkpoint or publication CI. Separately compiled
`-O3 -DNDEBUG` versions of each changed production TU passed the new groups;
these are optimized-TU checks, not full Release builds. Four copied production
variants each exited 1 at the intended check: omitted faction validation,
omitted saved-selection validation, crossed renderer admission, and use of the
current rather than downloaded selection. Intended source was unchanged during
the controls. Local commands, logs and hashes are retained under
`/home/jhurliman/src/glsmac-selected-rover/`. Independent source/fixture review
was read-only and did not rerun these checks.

No broader claim about live movement atomicity,
allocation-failure transactional spawning, arbitrary designs/faction materials,
rendered reload, or playable default startup follows from this prerequisite.
