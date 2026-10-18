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

The production `Unit` now has a distinct native variant. Its version5/kind2
record envelope preserves the complete native payload and uses an absent
controller sentinel independently of native owner0 and ecology attribution.
Conventional and legacy Units keep their existing version4 bytes. Decode checks
the world coordinate, canonical presentation definition, source-pool identity,
and the admitted living, homeless singleton state before claiming a tile.

Native presentation queries resolve Infantry/Psi/reactor components and movement
quantum from the frozen source catalog. They use the current global turn for
neutral display level; the manager's explicit opposing-faction query reads the
retained major-atrocity word. No borrowed conventional retained MORALE, elite
movement increment, maximum-health field or mutable legacy float is introduced.

This is an integration checkpoint, not completed ecology creation. The complete
factory still needs owning registry/pool allocation, native initialization and
counters, event-updated visibility, viewer-context badges, frontend ownership,
and atomic publication with bloom terrain/history/RNG. Full campaign restore
with those live entities remains unverified. PR289 stays draft until those
production paths and their gameplay validation are connected.

Validation: cached incremental build and six focused native groups pass,
including the production Unit decoder/encoder and presentation query over all
ten original complete birth records, existing conventional malformed-record
controls and private-context snapshot restoration. The old record byte-equality
check caught an unnecessary conventional envelope change; version4 output is
retained. Targeted correctness analysis passes Unit.cpp, NativeUnit.cpp and
UnitManager.cpp. The same six groups pass ASan/UBSan with those three production
TUs plus SmacNativeUnitRecord.cpp instrumented; remaining normal objects are
reused and leak detection is disabled. No platform matrix was repeated locally.
