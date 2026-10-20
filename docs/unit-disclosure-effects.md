# Unit disclosure event effects

The original disclosure routines clear Unit flags `0x440` for a positive-faction
observer, even when its stored visibility bit was already present. Faction0
sight does not clear the pair. The [original controls](behavior/conventional-unit-flags16-authority.md)
keep that mutation separate from mask storage, relocation and ownership changes.

`PreparedConventionalSight` now owns the proposed knowledge plus the identities
actually disclosed to positive factions. Preparation and discarded plans leave
live flags untouched. Commit applies the saved knowledge and clears only the
pair on those surviving Units. Known flags retain every unrelated bit; missing
older-port words remain unknown. Existing stored masks, technology reveal,
work-memory refresh and restore do not reconstruct disclosure events.

The event effects follow admitted conventional movement, newly disclosed ZOC
refusals, ordinary factories, restart and capture plans. Capture retains them
through its temporary territory projection and applies them only at final
commit. Native birth/encounter plans separately retain conventional stack flags
alongside native records; their temporary query view restores the previous flags
on return or exception. Existing detection restrictions remain explicit: this
change does not establish sensor, concealment or other unsupported detection.

Validation: 144 affected native groups pass across conventional movement, work,
Hold, manufacturing, capture, restart, native entities and actual restore. Seven
focused sight/native-stack groups also pass with GameWorkerKnowledge.cpp,
UnitManager.cpp, NativeUnit.cpp, ConventionalMovement.cpp, BaseCapture.cpp and
SmacLiveWorkerKnowledge.cpp instrumented under ASan/UBSan. Other objects remain
uninstrumented; leak detection is disabled. Correctness analysis passes for all
five changed production translation units.

The strengthened actual-consumer tests check repeated disclosure with an already
set mask, unrelated flag-bit preservation, unknown migrated flags, preparation
and discard without mutation, ZOC's new-disclosure distinction, and movement
away/save/reload without replaying accumulated stored visibility. This is not a
claim that ordinary main-map fog or actor gates are wired yet. Duration-byte
expiry and editor mutations retain their separate source-backed requirements.
