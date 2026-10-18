# Complete native Unit records and wild Mind Worm levels

`SmacNativeUnitRecord` retains every semantic field of the original 52-byte
Unit record, including auxiliary controls, waypoints, movement/order fields,
observation, independent ecology attribution and signed home/stack sentinels.
Its versioned portable codec uses named scalar fields, not a packed host layout.
Source simulation-pool ID0 and owner0 are valid; -1 is absence in signed links.
These pool identities must be bound explicitly to GLSMAC registry identities;
they are not aliases for a human controller or legacy manager slot.

The Mind Worm initializer stages the original unplaced -4/-4 record and wraps
the native direct morale scalar plus1 through the byte domain. It does not run
the factory's visibility/placement, optional AI assignment or owning counters.
A saved record preserves nonzero retained controls without inventing their
unmeasured AI meaning. A live snapshot binder must validate world references
and admitted native designs before publishing it.

The wild design8 level query uses current global turn rather than stored
morale or ecology attribution, and applies the separate opposing-faction
nonzero-atrocity modifier with clamp6. Unmeasured special state/flag markers
and changed ownership reject this baseline query rather than silently using
ordinary faction morale. Full movement allowance, health and combat belong to
the resolved native design and their own consumers; no per-Unit reactor or
maximum-HP default is invented here.

Source controls are the [complete original records and save/load queries](ecology-native-unit-records-cases.json)
and [initialized native birth/lifecycle controls](native-birth-producer-cases.json).
Tests cover all ten literal completed ecology records, native ID0, complete
portable roundtrips, independent nonzero fields, signed/unsigned narrowing,
wire boundaries, direct-morale byte controls and existing-Unit lifecycle
boundaries. This adds detached native state; the live manager/visibility/
render/save integration, native AI and complete ecology handler remain open.

Validation: three focused native groups pass from the cached build. Targeted
correctness analysis reports zero failures for the new production TU. The same
three groups pass ASan/UBSan with that TU instrumented, other normal objects
reused and leak checking disabled. No platform matrix was repeated locally.
